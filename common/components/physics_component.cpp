#include "physics_component.h"

namespace fd {

PhysicsComponent::PhysicsComponent(Physics* pPhys, PhysicsShape* shape)
    : m_pPhysics(pPhys)
    , m_pShape(shape)
{
  assert(shape != NULL);
  m_shapeAppliedMovement = m_pShape->UseShapeAppliedMovement();
}

void PhysicsComponent::OnConnected() {
  static std::string BDATpos("position");
  static std::string BDATorient("orientation");
  static std::string BDATvelocity("velocity");
      
  if(!m_ownerBus->GetOwnerData(BDATorient, true, &m_pOwnerOrientation)
      || !m_ownerBus->GetOwnerData(BDATpos, true, &m_pOwnerPosition)
      || !m_ownerBus->GetOwnerData(BDATvelocity, true, &m_pOwnerVelocity)) {
    assert(false);
    SelfDestruct();
  }

  static std::string BDATpushVelocity("pushVelocity"); //already don't remember what BDAT stands for
  if(!m_ownerBus->GetOwnerData(BDATpushVelocity, true, &m_pOwnerPushVelocity)) {
    m_pOwnerPushVelocity = NULL; // this is ok
  }
  static std::string BDATcollideFlag("collidingLastFrame"); // bumblefuck data asinine tag?
  if(!m_ownerBus->GetOwnerData(BDATcollideFlag, true, &m_pOwnerCollidingLastFrame)) {
    m_pOwnerCollidingLastFrame = NULL;
  }

  RegisterSignal(std::string("Step"), this, &PhysicsComponent::OnStepSignal);
  // This is a bad sign... Should we do a typeid system?
  RegisterSignal(std::string("DestroyPhysics"), (Component*)this, &Component::SelfDestruct);
  RegisterSignal(std::string("AddImpulse"), this, &PhysicsComponent::OnImpulse);
  RegisterSignal(std::string("inputJump"), this, &PhysicsComponent::OnJump);
}
    
void PhysicsComponent::OnJump(float frameTime) {
  if(m_pOwnerCollidingLastFrame) {
    if(!(*m_pOwnerCollidingLastFrame)) {
      return; // don't air jump
    }
  }

  if(m_jumpCountdown > 0.0f) {
    return; // don't double jump
  }

  const float jumpAmount = -0.75f; // negative to go up
  Vec4f impulse = m_pPhysics->m_gravity * jumpAmount;

  const float jumpTime = 1.0f;
  m_jumpCountdown = jumpTime;
  *m_pOwnerVelocity += impulse;
}

void PhysicsComponent::OnImpulse(const Vec4f& impulse) {
  *m_pOwnerVelocity += impulse;
}

void PhysicsComponent::ShapeMovement(
    float deltatime, bool& hadGroundCollision) {
  hadGroundCollision = m_pShape->ApplyMovement(deltatime, *m_pOwnerVelocity,
      *m_pOwnerOrientation, *m_pOwnerPosition,
      hadGroundCollision);
}

// so the supposition is that we start out not colliding
// then add velocity to position to get test position
// while collision
//   backup to collision point,
//   use up that amount of delta time
//   reproject remaining frame velocity along collision normal
//   get new test position
// store off position as real
void PhysicsComponent::MultiStepMovement(
    float deltatime, bool& hadGroundCollision) {
  float remainingDelta = deltatime;
  Vec4f remainingVelocity = (*m_pOwnerVelocity) * remainingDelta;
  Vec4f currentPosition = *m_pOwnerPosition; 
  Mat4f& testOrientation = *m_pOwnerOrientation;
  
  Vec4f testPosition = currentPosition;
  for(int s = 0; s < 4; s++) { // only allow 4 substeps
    testPosition = testPosition + remainingVelocity;

    Vec4f hitNormal;
    Vec4f hitPos;
    Vec4f safePos;
    float attemptDeltatime = remainingDelta;
    if(m_pShape->DoesCollide(attemptDeltatime, testOrientation, testPosition,
        safePos, hitPos, hitNormal)) {
      float velAmount = remainingVelocity.length();
      Vec4f usedVector = (safePos - testPosition);
      float usedAmount = usedVector.length();
      testPosition = safePos;

      float velDotNormal = remainingVelocity.dot(hitNormal);
      // -1 as we expect
      Vec4f velAlongNormal = hitNormal * (-1.0f * velDotNormal);
      //Vec4f slideDir = remainingVelocity + 

      testPosition = safePos;
      break;



    } else {
      break;
    }
  }  
  *m_pOwnerPosition = testPosition;
}

void PhysicsComponent::SingleStepMovement(
    float deltatime, bool& hadGroundCollision) {

  Vec4f frameVelocity = *m_pOwnerVelocity * deltatime;

  // yeah it's becoming quickly clear that a slow progression of physics bits
  // is the wrong way to proceed. Should just do a proper decoupled system
  // where there are rigidbody + shape things in a physics scene.
  Vec4f possibleVelocity;
  hadGroundCollision = false;
  Vec4f hitNormal;
  Vec4f hitPos;
  if(m_pShape->DoesMovementCollide(*m_pOwnerOrientation, *m_pOwnerPosition,
      *m_pOwnerVelocity, deltatime, hitPos, possibleVelocity, hitNormal)) {
    frameVelocity = possibleVelocity * deltatime;
    *m_pOwnerVelocity = possibleVelocity;
    const float groundCollisionThreshold = 0.1f;
    if(hitNormal.dot(m_pPhysics->m_groundNormal) > groundCollisionThreshold) {
      hadGroundCollision = true;
    }
  }
  *m_pOwnerPosition += frameVelocity;
}

// yeah it's becoming quickly clear that a slow progression of physics bits
// is the wrong way to proceed. Should just do a proper decoupled system
// where there are rigidbody + shape things in a physics scene.    
void PhysicsComponent::OnStepSignal(float delta) {
  *m_pOwnerVelocity += m_pPhysics->m_gravity * delta;
  if(m_pOwnerPushVelocity) {
    *m_pOwnerVelocity += *m_pOwnerPushVelocity;
    *m_pOwnerPushVelocity = Vec4f(0,0,0,0);
  }

  bool hadGroundCollision = false;
  if(m_shapeAppliedMovement) {
    ShapeMovement(delta, hadGroundCollision);
  } else {
    SingleStepMovement(delta, hadGroundCollision);
    //MultiStepMovement(delta, hadGroundCollision);
  }

  if(m_jumpCountdown > 0.0) {
    m_jumpCountdown -= delta;
  }
      
  // the abundance of physics issues makes this a nice thing
  if(m_pPhysics->ClampToGround(m_pOwnerPosition, m_pOwnerVelocity)) {
    hadGroundCollision = true;
  }
  if(m_pOwnerCollidingLastFrame) {
    *m_pOwnerCollidingLastFrame = hadGroundCollision;
  }

  if(hadGroundCollision) {
    static float frictionCoef = -10.0f;
    *m_pOwnerVelocity += ((*m_pOwnerVelocity) * (frictionCoef * delta));
  }
}

} // namespace fd