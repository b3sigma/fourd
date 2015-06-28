#pragma once

#include "fourmath.h"

namespace fd {

class PhysicsShape;

class RigidBody {
  Mat4f m_orientation;
  Vec4f m_position;
  Vec4f m_velocity;

  PhysicsShape* m_shape; 
    
};


};