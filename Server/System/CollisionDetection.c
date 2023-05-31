#include <Server/System/CollisionDetection.h>

#include <stdio.h>
#include <string.h>

#include <Server/Simulation.h>
#include <Server/SpatialHash.h>
#include <Shared/Bitset.h>

struct physics_simulation_captures
{
    struct rr_simulation *simulation;
    struct rr_component_physical *physical;
};

static void system_reset_colliding_with(EntityIdx entity, void *captures)
{
    struct rr_simulation *this = captures;
    if (!rr_simulation_has_physical(this, entity))
        return;

    struct rr_component_physical *physical = rr_simulation_get_physical(this, entity);
    memset(&physical->collisions[0], 0, RR_BITSET_ROUND(RR_MAX_ENTITY_COUNT));
    physical->has_collisions = 0;
}

static void system_insert_entities(EntityIdx entity, void *_captures)
{
    struct rr_simulation *this = _captures;
    struct rr_component_physical *physical = rr_simulation_get_physical(this, entity);

    rr_spatial_hash_insert(this->grid, entity);
}

static void grid_filter_candidates(struct rr_simulation *this, EntityIdx entity1, EntityIdx entity2, void *_captures)
{
    struct rr_component_physical *physical1 = rr_simulation_get_physical(this, entity1);
    struct rr_component_physical *physical2 = rr_simulation_get_physical(this, entity2);
    struct rr_vector delta = {physical1->x - physical2->x, physical1->y - physical2->y};
    float collision_radius = physical1->radius + physical2->radius;
    if ((delta.x * delta.x + delta.y * delta.y) < collision_radius * collision_radius)
    {
        // if (entity1 < entity2) // a very sneaky optimization
        {
            // puts("found collision!!");
            physical1->has_collisions = 1;
            physical2->has_collisions = 1;
            rr_bitset_set(physical1->collisions, entity2);
            rr_bitset_set(physical2->collisions, entity1);
        }
    }
}

static void find_collisions(struct rr_simulation *this)
{
    rr_spatial_hash_find_possible_collisions(this->grid, NULL, grid_filter_candidates);
}

static void system_find_collisions(EntityIdx entity1, void *_captures)
{
    struct rr_simulation *this = _captures;
    struct rr_component_physical *physical1 = rr_simulation_get_physical(this, entity1);

    // uint8_t colliding_with[RR_BITSET_ROUND(RR_MAX_ENTITY_COUNT)] = {};
    // null terminated array
    EntityIdx colliding_with[RR_MAX_ENTITY_COUNT + 1] = {};

    // rr_spatial_hash_query(this->grid, physical1->x, physical1->y, physical1->radius, physical1->radius, colliding_with);
    // rr_spatial_hash_query(this->grid, physical1->x, physical1->y, physical1->radius, physical1->radius, physical1, grid_filter_candidates);

    // struct physics_simulation_captures captures;
    // captures.simulation = this;
    // captures.physical = physical1;

    // rr_bitset_for_each_bit(colliding_with, colliding_with + RR_BITSET_ROUND(RR_MAX_ENTITY_COUNT), &captures, grid_filter_candidates);

    // for (uint64_t i = 0;; i++)
    // {
    //     if (colliding_with[i] == RR_NULL_ENTITY)
    //         break;
    //     EntityIdx entity2 = colliding_with[i];
    //     if (entity1 == entity2)
    //         continue;

    //     struct rr_component_physical *physical2 = rr_simulation_get_physical(this, entity2);
    //     struct rr_vector delta = {physical1->x - physical2->x, physical1->y - physical2->y};
    //     float collision_radius = physical1->radius + physical2->radius;
    //     if ((delta.x * delta.x + delta.y * delta.y) < collision_radius * collision_radius)
    //     {
    //         if (entity1 < entity2) // a very sneaky optimization
    //         {
    //             physical1->has_collisions = 1;
    //             physical2->has_collisions = 1;
    //             rr_bitset_set(physical1->collisions, entity2);
    //         }
    //     }
    // }

    // for (uint64_t i = 0; i < RR_MAX_ENTITY_COUNT; i++)
    // {
    //     if (rr_bitset_get_bit(colliding_with, i))
    //     {
    //         EntityIdx entity2 = i;
    //         if (entity1 == entity2)
    //             continue;

    //         struct rr_component_physical *physical2 = rr_simulation_get_physical(this, entity2);
    //         struct rr_vector delta = {physical1->x - physical2->x, physical1->y - physical2->y};
    //         float collision_radius = physical1->radius + physical2->radius;
    //         if ((delta.x * delta.x + delta.y * delta.y) < collision_radius * collision_radius)
    //         {
    //             if (!rr_bitset_get_bit(physical1->collisions, entity1) && !rr_bitset_get_bit(physical2->collisions, entity1) && !rr_bitset_get_bit(physical1->collisions, entity2) && !rr_bitset_get_bit(physical2->collisions, entity2))
    //             {
    //                 rr_bitset_set(physical1->collisions, entity2);
    //             }
    //         }
    //     }
    // }
}

void rr_system_collision_detection_tick(struct rr_simulation *this)
{
    rr_simulation_for_each_physical(this, this, system_reset_colliding_with);
    rr_simulation_for_each_physical(this, this, system_insert_entities);
    // rr_simulation_for_each_physical(this, this, system_find_collisions);
    find_collisions(this);
}
