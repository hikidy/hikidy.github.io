#include <Server/System/PetalBehavior.h>

#include <math.h>
#include <stdio.h>

#include <Server/EntityAllocation.h>
#include <Server/Simulation.h>

#include <Shared/Entity.h>
#include <Shared/StaticData.h>
#include <Shared/Utilities.h>
#include <Shared/Vector.h>

struct uranium_captures
{
    struct rr_simulation *simulation;
    EntityIdx flower_id;
    float x;
    float y;
    float damage;
};

static void uranium_damage(EntityIdx mob, void *_captures)
{
    struct uranium_captures *captures = _captures;
    struct rr_simulation *simulation = captures->simulation;
    if (rr_simulation_get_relations(simulation, mob)->team ==
        rr_simulation_team_id_players)
        return;
    struct rr_component_health *health =
        rr_simulation_get_health(simulation, mob);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, mob);
    if ((physical->x - captures->x) * (physical->x - captures->x) +
            (physical->y - captures->y) * (physical->y - captures->y) <
        901 * 901)
    {
        rr_component_health_do_damage(health, captures->damage);
        // health->damage_paused = 5;
        struct rr_component_ai *ai = rr_simulation_get_ai(simulation, mob);
        if (ai->target_entity == RR_NULL_ENTITY)
            ai->target_entity = captures->flower_id;
    }
}

static void uranium_petal_system(struct rr_simulation *simulation,
                                 struct rr_component_petal *petal)
{
    if (--petal->effect_delay == 0)
    {
        struct rr_component_relations *relations =
            rr_simulation_get_relations(simulation, petal->parent_id);
        struct rr_component_health *health =
            rr_simulation_get_health(simulation, petal->parent_id);
        struct rr_component_physical *physical =
            rr_simulation_get_physical(simulation, petal->parent_id);
        if (!rr_simulation_has_entity(simulation, relations->owner))
            return;
        struct rr_component_physical *flower_physical =
            rr_simulation_get_physical(simulation, relations->owner);
        rr_component_health_set_health(health, health->health -
                                                          health->damage * 1.5);
        petal->effect_delay = 25;
        struct uranium_captures captures = {simulation, relations->owner,
                                            physical->x, physical->y,
                                            health->damage};
        rr_simulation_for_each_mob(simulation, &captures, uranium_damage);
    }
}

static void system_petal_detach(struct rr_simulation *simulation,
                                struct rr_component_petal *petal,
                                struct rr_component_player_info *player_info,
                                uint32_t outer_pos, uint32_t inner_pos,
                                struct rr_petal_data const *petal_data)
{
    rr_component_petal_set_detached(petal, 1);
    struct rr_component_player_info_petal *ppetal =
        &player_info->slots[outer_pos].petals[inner_pos];
    ppetal->simulation_id = RR_NULL_ENTITY;
    ppetal->cooldown_ticks = petal_data->cooldown;
}

static void system_flower_petal_movement_logic(
    struct rr_simulation *simulation, EntityIdx id,
    struct rr_component_player_info *player_info, uint32_t rotation_pos,
    uint32_t outer_pos, uint32_t inner_pos,
    struct rr_petal_data const *petal_data)
{
    struct rr_component_petal *petal = rr_simulation_get_petal(simulation, id);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, id);
    struct rr_component_physical *flower_physical =
        rr_simulation_get_physical(simulation, player_info->flower_id);
    struct rr_vector position_vector = {physical->x, physical->y};
    struct rr_vector flower_vector = {flower_physical->x, flower_physical->y};
    float curr_angle = player_info->global_rotation +
                       rotation_pos * 2 * M_PI / player_info->rotation_count;
    uint8_t is_projectile = rr_simulation_has_projectile(simulation, id);
    if (is_projectile)
    {
        struct rr_component_projectile *projectile =
            rr_simulation_get_projectile(simulation, id);
        if (petal->effect_delay <= 0)
        {
            switch (petal->id)
            {
            case rr_petal_id_missile:
            {
                if ((player_info->input & 1) == 0)
                    break;
                system_petal_detach(simulation, petal, player_info, outer_pos,
                                    inner_pos, petal_data);
                rr_vector_from_polar(&physical->acceleration, 10.0f,
                                     physical->angle);
                rr_vector_from_polar(&physical->velocity, 50.0f,
                                     physical->angle);
                projectile->ticks_until_death = 75;
                rr_simulation_get_health(simulation, id)->damage =
                    25 * RR_PETAL_RARITY_SCALE[petal->rarity].damage;
                break;
            }
            case rr_petal_id_peas:
            {
                if ((player_info->input & 1) == 0)
                    break;
                system_petal_detach(simulation, petal, player_info, outer_pos,
                                    inner_pos, petal_data);
                rr_vector_from_polar(&physical->acceleration, 4.0f,
                                     physical->angle);
                rr_vector_from_polar(&physical->velocity, 50.0f,
                                     physical->angle);
                projectile->ticks_until_death = 15;
                uint32_t count = petal_data->count[petal->rarity];
                for (uint32_t i = 1; i < count; ++i)
                {
                    EntityIdx new_petal = rr_simulation_alloc_petal(
                        simulation, physical->x, physical->y, petal->id, petal->rarity,
                        flower_physical->parent_id);
                    struct rr_component_physical *new_physical =
                        rr_simulation_get_physical(simulation, new_petal);
                    rr_component_physical_set_angle(
                        new_physical, physical->angle + i * 2 * M_PI / count);
                    rr_vector_from_polar(&new_physical->acceleration, 4.0f,
                                         new_physical->angle);
                    rr_vector_from_polar(&new_physical->velocity, 50.0f,
                                         new_physical->angle);
                    rr_component_petal_set_detached(
                        rr_simulation_get_petal(simulation, new_petal), 1);
                    struct rr_component_projectile *new_projectile =
                        rr_simulation_get_projectile(simulation, new_petal);
                    new_projectile->ticks_until_death = 15;
                }
                break;
            }
            case rr_petal_id_azalea:
            {
                struct rr_component_health *flower_health =
                    rr_simulation_get_health(simulation,
                                             player_info->flower_id);
                if (flower_health->health < flower_health->max_health)
                {
                    struct rr_vector delta = {
                        (flower_vector.x - position_vector.x),
                        (flower_vector.y - position_vector.y)};
                    if (rr_vector_get_magnitude(&delta) <
                        flower_physical->radius + physical->radius)
                    {
                        // heal
                        rr_component_health_set_health(
                            flower_health,
                            flower_health->health +
                                10 * RR_PETAL_RARITY_SCALE[petal->rarity]
                                         .damage);
                        rr_simulation_request_entity_deletion(simulation, id);
                        return;
                    }
                    else
                    {
                        rr_vector_scale(&delta, 0.4);
                        rr_vector_add(&physical->acceleration, &delta);
                        return;
                    }
                }
                else
                {
                    for (uint32_t i = 0; i < simulation->flower_count; ++i)
                    {
                        EntityIdx potential = simulation->flower_vector[i];
                        struct rr_component_physical *target_physical =
                            rr_simulation_get_physical(simulation, potential);
                        struct rr_vector delta = {
                            (target_physical->x - position_vector.x),
                            (target_physical->y - position_vector.y)};
                        if (rr_vector_get_magnitude(&delta) > 200)
                            continue;
                        flower_health =
                            rr_simulation_get_health(simulation, potential);
                        if (flower_health->health == flower_health->max_health)
                            continue;
                        if (rr_vector_get_magnitude(&delta) <
                            target_physical->radius + physical->radius)
                        {
                            // heal
                            rr_component_health_set_health(
                                flower_health,
                                flower_health->health +
                                    10 * RR_PETAL_RARITY_SCALE[petal->rarity]
                                             .damage);
                            rr_simulation_request_entity_deletion(simulation,
                                                                  id);
                            return;
                        }
                        else
                        {
                            rr_vector_scale(&delta, 0.5);
                            rr_vector_add(&physical->acceleration, &delta);
                            return;
                        }
                    }
                }
                break;
            }
            case rr_petal_id_web:
            {
                if ((player_info->input & 3) == 0)
                    break;
                system_petal_detach(simulation, petal, player_info, outer_pos,
                                    inner_pos, petal_data);
                if (player_info->input & 1)
                {
                    float angle =
                        player_info->global_rotation +
                        rotation_pos * 2 * M_PI / player_info->rotation_count;
                    rr_vector_from_polar(&physical->acceleration, 7.5f,
                                         curr_angle);
                    rr_vector_from_polar(&physical->velocity, 50.0f,
                                         curr_angle);
                }
                projectile->ticks_until_death = 20;
                break;
            }
            case rr_petal_id_seed:
            {
                if (simulation->player_info_count > simulation->flower_count)
                {
                    if (!petal->detached)
                    {
                        projectile->ticks_until_death =
                            2700 / RR_PETAL_RARITY_SCALE[petal->rarity].damage;
                        rr_component_petal_set_detached(petal, 1);
                    }
                    return;
                }
                break;
            }
            case rr_petal_id_gravel:
            {
                if ((player_info->input & 3) == 0)
                    break;
                system_petal_detach(simulation, petal, player_info, outer_pos,
                                    inner_pos, petal_data);
                projectile->ticks_until_death = 125;
                physical->friction = 0.4;
                break;
            }
            case rr_petal_id_lightning:
            {
                if ((player_info->input & 1) == 0)
                    break;
                system_petal_detach(simulation, petal, player_info, outer_pos,
                                    inner_pos, petal_data);
                rr_vector_from_polar(&physical->acceleration, 15.0f,
                                     physical->angle);
                rr_vector_from_polar(&physical->velocity, 100.0f,
                                     physical->angle);
                projectile->ticks_until_death = 8;
                rr_simulation_get_health(simulation, id)->damage =
                    25 * RR_PETAL_RARITY_SCALE[petal->rarity].damage;
                break;
            }
            default:
                break;
            }
        }
        else
            --petal->effect_delay;
    }

    float holdingRadius = 75;
    uint8_t should_extend = player_info->input & 1 && !is_projectile &&
        petal_data->id != rr_petal_id_uranium &&
        petal_data->id != rr_petal_id_magnet &&
        petal_data->id != rr_petal_id_bone;
    if (petal->id == rr_petal_id_gravel && petal->detached)
        should_extend = player_info->input & 1;
    if (should_extend)
        holdingRadius = 150;
    else if (player_info->input & 2)
        holdingRadius = 45;
    struct rr_vector chase_vector;
    rr_vector_from_polar(&chase_vector, holdingRadius, curr_angle);
    rr_vector_add(&chase_vector, &flower_vector);
    rr_vector_sub(&chase_vector, &position_vector);
    if (petal_data->clump_radius != 0.0f &&
        petal_data->count[petal->rarity] != 1)
    {
        curr_angle = 1.333 * curr_angle +
                     2 * M_PI * inner_pos / petal_data->count[petal->rarity];
        struct rr_vector clump_vector;
        rr_vector_from_polar(&clump_vector, petal_data->clump_radius,
                             curr_angle);
        rr_vector_add(&chase_vector, &clump_vector);
    }
    if (petal->id == rr_petal_id_light)
    {
        struct rr_vector random_vector;
        rr_vector_from_polar(&random_vector, 10.0f, rr_frand() * M_PI * 2);
        rr_vector_add(&chase_vector, &random_vector);
    }
    if (!is_projectile || petal->id == rr_petal_id_seed ||
        petal->id == rr_petal_id_web || petal->id == rr_petal_id_peas)
        rr_component_physical_set_angle(
            physical, physical->angle + 0.04f * (float)petal->spin_ccw);
    else
        rr_component_physical_set_angle(physical, curr_angle);
    physical->acceleration.x += 0.5f * chase_vector.x;
    physical->acceleration.y += 0.5f * chase_vector.y;
}

static void petal_modifiers(struct rr_simulation *simulation,
                            struct rr_component_player_info *player_info)
{
    struct rr_component_flower *flower =
        rr_simulation_get_flower(simulation, player_info->flower_id);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, player_info->flower_id);
    struct rr_component_health *health =
        rr_simulation_get_health(simulation, player_info->flower_id);
    rr_component_flower_set_face_flags(flower, player_info->input);
    // reset
    physical->acceleration_scale = 1;
    player_info->modifiers.drop_pickup_radius = 25;
    player_info->modifiers.rotation_direction = 1;
    uint8_t rot_count = 0;
    rr_component_player_info_set_camera_fov(player_info, 1.0f);
    health->damage_reduction = 0;
    float to_rotate = 0.1;
    for (uint64_t outer = 0; outer < player_info->slot_count; ++outer)
    {
        struct rr_component_player_info_petal_slot *slot =
            &player_info->slots[outer];
        struct rr_petal_data const *data = &RR_PETAL_DATA[slot->id];
        if (data->id == rr_petal_id_leaf)
        {
            rr_component_health_set_health(
                health, health->health +
                            0.05 * RR_PETAL_RARITY_SCALE[slot->rarity].damage);
        }
        else if (data->id == rr_petal_id_light)
            to_rotate += (0.012 + 0.008 * slot->rarity);
        else if (data->id == rr_petal_id_feather)
        {
            float speed = 1 + 0.05 + 0.035 * slot->rarity;
            if (speed > physical->acceleration_scale)
                physical->acceleration_scale = speed;
        }
        else if (data->id == rr_petal_id_crest)
        {
            rr_component_flower_set_face_flags(flower, flower->face_flags | 8);
            if (player_info->camera_fov > 1.0f * (0.9 - 0.05 * slot->rarity))
            {   
                rr_component_player_info_set_camera_fov(player_info, 1.0f * (0.9 - 0.06 * slot->rarity));
            }
        }
        else if (data->id == rr_petal_id_droplet)
        {
           ++rot_count;
        }
        else
            for (uint32_t inner = 0; inner < slot->count; ++inner)
            {
                if (slot->petals[inner].simulation_id == RR_NULL_ENTITY)
                    continue;
                if (data->id == rr_petal_id_magnet)
                {
                    player_info->modifiers.drop_pickup_radius +=
                        -25 + data->id * 25;
                }
                if (data->id == rr_petal_id_bone)
                    health->damage_reduction +=
                        2.5 * RR_PETAL_RARITY_SCALE[slot->rarity].health;
            }
    }
    player_info->global_rotation += to_rotate * ((rot_count % 3) ? (rot_count % 3 == 2) ? 0 : -1 : 1);
}

static void rr_system_petal_reload_foreach_function(EntityIdx id,
                                                    void *simulation)
{
    struct rr_component_player_info *player_info =
        rr_simulation_get_player_info(simulation, id);
    if (player_info->flower_id == RR_NULL_ENTITY)
        return;
    petal_modifiers(simulation, player_info);
    uint32_t rotation_pos = 0;
    for (uint64_t outer = 0; outer < player_info->slot_count; ++outer)
    {
        struct rr_component_player_info_petal_slot *slot =
            &player_info->slots[outer];
        struct rr_petal_data const *data = &RR_PETAL_DATA[slot->id];
        uint8_t max_cd = 0;
        slot->count = slot->id == rr_petal_id_peas
                          ? 1
                          : RR_PETAL_DATA[slot->id].count[slot->rarity];
        for (uint64_t inner = 0; inner < slot->count; ++inner)
        {
            if (inner == 0 || data->clump_radius == 0)
                ++rotation_pos; // clump rotpos ++
            struct rr_component_player_info_petal *p_petal =
                &slot->petals[inner];
            if (p_petal->simulation_id != RR_NULL_ENTITY &&
                !rr_simulation_has_entity(simulation,
                                           p_petal->simulation_id))
            {
                p_petal->simulation_id = RR_NULL_ENTITY;
                p_petal->cooldown_ticks = data->cooldown;
            }
            if (p_petal->simulation_id == RR_NULL_ENTITY)
            {
                float cd =
                    ((float)p_petal->cooldown_ticks / data->cooldown) * 255;
                if (cd > max_cd)
                {
                    if (cd > 255)
                        cd = 255;
                    max_cd = (uint8_t)cd;
                }
                if (--p_petal->cooldown_ticks <= 0)
                {
                    p_petal->simulation_id = rr_simulation_alloc_petal(
                        simulation, player_info->camera_x, player_info->camera_y, slot->id, slot->rarity,
                        player_info->flower_id);
                }
            }
            else
            {
                if (rr_simulation_has_mob(simulation, p_petal->simulation_id))
                {
                    if (inner == 0 || data->clump_radius == 0)
                        --rotation_pos; // clump rotpos --
                    continue;           // player spawned mob
                }
                system_flower_petal_movement_logic(
                    simulation, p_petal->simulation_id, player_info,
                    rotation_pos, outer, inner, data);
                if (data->id == rr_petal_id_egg)
                {
                    struct rr_component_petal *petal = rr_simulation_get_petal(
                        simulation, p_petal->simulation_id);
                    if (petal->effect_delay > 0)
                        continue;
                    rr_component_petal_set_detached(petal, 1);
                    struct rr_component_physical *petal_physical =
                        rr_simulation_get_physical(simulation,
                                                   p_petal->simulation_id);
                    rr_simulation_request_entity_deletion(
                        simulation, p_petal->simulation_id);
                    p_petal->simulation_id = RR_NULL_ENTITY;
                    if (petal->rarity == 0)
                        return;
                    EntityIdx mob_id = p_petal->simulation_id =
                        rr_simulation_alloc_mob(simulation, petal_physical->x, petal_physical->y, rr_mob_id_trex,
                                                petal->rarity - 1,
                                                rr_simulation_team_id_players);
                    struct rr_component_relations *relations =
                        rr_simulation_get_relations(simulation, mob_id);
                    rr_component_relations_set_owner(relations,
                                                     player_info->flower_id);
                    rr_simulation_get_mob(simulation, mob_id)->player_spawned = 1;
                }
            }
        }
        rr_component_player_info_set_slot_cd(player_info, outer, max_cd);
    }
    player_info->rotation_count = rotation_pos;
}

static void system_petal_misc_logic(EntityIdx id, void *_simulation)
{
    struct rr_simulation *simulation = _simulation;
    struct rr_component_petal *petal = rr_simulation_get_petal(simulation, id);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, id);
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, id);
    if (petal->detached == 0) // it's mob owned if this is true
    {
        if (petal->id == rr_petal_id_uranium)
            uranium_petal_system(simulation, petal);
        if (!rr_simulation_has_entity(simulation, relations->owner))
        {
            rr_simulation_request_entity_deletion(simulation, id);
            return;
        }
        if (!rr_simulation_has_mob(simulation, relations->owner))
        {
            return;
        }
        // check if owner is a mob
        return; // no logic yet
    }
    else
    {
        if (petal->id == rr_petal_id_missile)
            rr_vector_from_polar(&physical->acceleration, 10.0f,
                                 physical->angle);
        else if (petal->id == rr_petal_id_peas)
            rr_vector_from_polar(&physical->acceleration, 7.5f,
                                 physical->angle);
        else if (petal->id == rr_petal_id_seed)
        {
            if (simulation->player_info_count <= simulation->flower_count)
                rr_simulation_request_entity_deletion(simulation, id);
        }
        else if (petal->id == rr_petal_id_lightning)
            rr_vector_from_polar(&physical->acceleration, 15.0f,
                                 physical->angle);
        if (--rr_simulation_get_projectile(simulation, id)->ticks_until_death <=
            0)
        {
            rr_simulation_request_entity_deletion(simulation, id);
            if (petal->id == rr_petal_id_lightning)
            {
                struct rr_component_physical *physical = rr_simulation_get_physical(simulation, id);
                struct rr_simulation_animation *animation = &simulation->animations[simulation->animation_length++];
                animation->type = 1;
                EntityIdx chain[16] = {0};
                animation->points[0].x = physical->x;
                animation->points[0].y = physical->y;
                uint32_t chain_size = 1;
                uint32_t chain_amount = petal->rarity + 2;   
                float damage = rr_simulation_get_health(simulation, id)->damage * 0.5; 
                EntityIdx target = RR_NULL_ENTITY;
            
                for (; chain_size < chain_amount + 1; ++chain_size)
                {
                    float old_x = physical->x, old_y = physical->y;
                    target = RR_NULL_ENTITY;
                    float min_dist = 500 * 500;
                    if (relations->team == rr_simulation_team_id_players)
                    {
                        for (uint16_t i = 0; i < simulation->mob_count; ++i)
                        {
                            EntityIdx mob_id = simulation->mob_vector[i];
                            uint8_t hit_before = 0;
                            for (uint32_t j = 0; j < chain_size; ++j)
                                hit_before |= chain[j] == mob_id;
                            if (hit_before)
                                continue;
                            if (rr_simulation_get_relations(simulation, mob_id)->team == rr_simulation_team_id_players)
                                continue;
                            physical = rr_simulation_get_physical(simulation, mob_id);
                            float x = physical->x, y = physical->y;
                            float dist = (x - old_x) * (x - old_x) + (y - old_y) * (y - old_y);
                            if (dist > min_dist)
                                continue;
                            target = mob_id;
                            min_dist = dist;
                        }
                    }
                    else
                    {
                        for (uint16_t i = 0; i < simulation->flower_count; ++i)
                        {
                            EntityIdx mob_id = simulation->flower_vector[i];
                            uint8_t hit_before = 0;
                            for (uint32_t j = 0; j < chain_size; ++j)
                                hit_before |= chain[j] == mob_id;
                            if (hit_before)
                                continue;
                            if (rr_simulation_get_relations(simulation, mob_id)->team == rr_simulation_team_id_mobs)
                                continue;
                            physical = rr_simulation_get_physical(simulation, mob_id);
                            float x = physical->x, y = physical->y;
                            float dist = (x - old_x) * (x - old_x) + (y - old_y) * (y - old_y);
                            if (dist > min_dist)
                                continue;
                            target = mob_id;
                            min_dist = dist;
                        }
                    }
                    if (target == RR_NULL_ENTITY)
                        break;
                    struct rr_component_health *health = rr_simulation_get_health(simulation, target);
                    rr_component_health_do_damage(health, damage);
                    health->damage_paused = 5;
                    chain[chain_size] = target;
                    physical = rr_simulation_get_physical(simulation, target);
                    animation->points[chain_size].x = physical->x;
                    animation->points[chain_size].y = physical->y;
                }
                animation->length = chain_size;
            }
            if (petal->id == rr_petal_id_seed)
            {
                for (uint32_t i = 0; i < simulation->player_info_count; ++i)
                {
                    if (rr_simulation_get_player_info(
                            simulation, simulation->player_info_vector[i])
                            ->flower_id != RR_NULL_ENTITY)
                        continue;
                    EntityIdx flower = rr_simulation_alloc_player(
                        simulation->player_info_vector[i], simulation);
                    struct rr_component_physical *flower_physical =
                        rr_simulation_get_physical(simulation, flower);
                    rr_component_physical_set_x(flower_physical, physical->x);
                    rr_component_physical_set_y(flower_physical, physical->y);
                    return;
                }
            }
        }
    }
}

void rr_system_petal_behavior_tick(struct rr_simulation *simulation)
{
    rr_simulation_for_each_player_info(simulation, simulation,
                                       rr_system_petal_reload_foreach_function);
    rr_simulation_for_each_petal(simulation, simulation,
                                 system_petal_misc_logic);
}
