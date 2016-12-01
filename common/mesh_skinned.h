#pragma once

#include <vector>
#include "fourmath.h"
#include "mesh.h"

namespace fd {

  // the idea behind this is to have a heirarchy of bones
  // the vertex will store its offset from the bone-space transform as position
  // the vertex will also have the bone index
  // the bone transformations are each a pose, which in the shader will be an array of mat4 and vec4 for rotation and position satisfying Rx + P
  // the vertex shader uses the vertex bone index to look up the bone transform
  // uses the bone transfrom to convert bone space position into object space
  // uses the object transform to convert object space into projection space as usual
  // before putting the bone transformations into the shader arrays, they must be converted from heirarchical form to absolute form by transforming by parent

  class MeshSkinned : public Mesh {
  public:
    struct Bone {
      enum { InvalidParentIndex = -1 };
      int _parentIndex; // -1 for root?
      Pose4f _pose;
    };

    typedef std::vector<Bone> VecBones;
    VecBones _bones;

    typedef std::vector<unsigned char> BoneIndexList;
    BoneIndexList _vertBoneIndices;

    void buildCactusDancer();

    virtual void clearCurrent();

  };

} // namespace fd
