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
    class Bone { public: ALIGNED_ALLOC_NEW_DEL_OVERRIDE
      Pose4f _poseRelative;
      enum { InvalidParentIndex = -1 };
      int _parentIndex = InvalidParentIndex; // InvalidParentIndex to indicate this is root
      int _dirtyCounter = 0; // currently only needs 1 bit, but used int for alignment


      //Bone() {}
      //Bone(const Bone& c)
      //  : _poseRelative(c._poseRelative)
      //  , _parentIndex(c._parentIndex)
      //  , _dirtyCounter(c._dirtyCounter) {} // needless, trying to get around weird alignment issue
    };

    virtual ~MeshSkinned() {}

    typedef std::vector<Bone, Eigen::aligned_allocator<Bone> > VecBones;
    VecBones _bones;

    typedef std::vector<unsigned char> BoneIndexList;
    BoneIndexList _vertBoneIndices;
    const unsigned char* getBoneIndex(int vertex) const { return &_vertBoneIndices[vertex]; }

    // this structure is more closely aligned with how the renderer will need it
    typedef std::vector<Vec4f> VecBonePositions;
    typedef std::vector<Mat4f, Eigen::aligned_allocator<Mat4f> > VecBoneRotations;
    // these are the full concatenated poses that map from local bone space to final object space
    VecBonePositions _bonePositions; 
    VecBoneRotations _boneRotations;

    void buildCactusDancer();
    void buildTilt();
    void buildAxisSignPost();


    void updateFullPoses();

    virtual void clearCurrent();

  protected:
    void updateBoneRecursive(int index, char dirtyCounter);
    void fillBonesWithTesseracts(float segmentLength);


  };

} // namespace fd
