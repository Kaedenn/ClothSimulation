#pragma once
#include <SFML/System/Vector2.hpp>
#include "../common/index_vector.hpp"


struct Particle
{
    civ::ID id = 0;
    float mass = 1.0f;
    sf::Vector2f position;
    sf::Vector2f position_old;
    sf::Vector2f velocity;
    sf::Vector2f forces;
    sf::Color color = sf::Color::White;
    bool moving = true;

    Particle() = default;

    explicit
    Particle(sf::Vector2f pos)
    : position(pos)
    , position_old(pos)
    {}

    Particle(float mass, sf::Vector2f pos)
    : mass(mass)
    , position(pos)
    , position_old(pos)
    {}

    void update(float dt)
    {
        if (!moving) return;
        position_old = position;
        velocity += (forces / mass) * dt;
        position += velocity * dt;
    }

    void updateDerivatives(float dt)
    {
        velocity = (position - position_old) / dt;
        forces = {};
    }

    void move(sf::Vector2f v)
    {
        if (!moving) return;
        position += v;
    }
};

using ParticleRef = civ::Ref<Particle>;

/* vim: set ts=4 sts=4 sw=4 et: */
