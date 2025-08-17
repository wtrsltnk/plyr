#ifndef ENTITIES_HPP
#define ENTITIES_HPP

struct MeshComponent
{
    size_t first;
    size_t count;
};

struct PhysicsComponent
{
    size_t bodyIndex = 0;
};


#endif // ENTITIES_HPP
