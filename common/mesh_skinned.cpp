#include <memory>
#include "camera.h"
#include "mesh_skinned.h"

namespace fd {

void MeshSkinned::buildCactusDancer() {
  // usually we might want the root elsewhere but in this case put it on the ground to make things easier to debug initially
  int numUpright = 4;
  float segmentLength = 10.0f;
  
  {
    Bone bone;
    bone._pose.storeIdentity();
    bone._parentIndex = Bone::InvalidParentIndex;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._pose.rotation.storeRotation((float)PI / 16.0f, Camera::UP, Camera::RIGHT);
    bone._pose.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._pose.rotation.storeRotation((float)PI / 16.0f, Camera::UP, Camera::FORWARD);
    bone._pose.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._pose.rotation.storeRotation((float)PI / 16.0f, Camera::UP, Camera::INSIDE);
    bone._pose.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  int branchBone = (int)_bones.size() - 1;
  {

    Bone bone;
    bone._pose.rotation.storeIdentity();
    bone._pose.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = branchBone;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._pose.rotation.storeRotation((float)PI / 2.0f, Camera::UP, Camera::RIGHT);
    bone._pose.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = branchBone;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._pose.rotation.storeIdentity();
    bone._pose.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._pose.rotation.storeRotation(-(float)PI / 2.0f, Camera::UP, Camera::RIGHT);
    bone._pose.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = branchBone;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._pose.rotation.storeIdentity();
    bone._pose.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  std::unique_ptr<Mesh> mesh(new Mesh());
  mesh->buildTesseract(segmentLength);

  _vertBoneIndices.resize(0);
  _vertBoneIndices.reserve(_bones.size() * mesh->_verts.size());
  for(int bone = 0; bone < _bones.size(); bone++) {
    merge(*mesh);

    for(auto vert : mesh->_verts) {
      _vertBoneIndices.push_back(bone);
    }
  }

}

void MeshSkinned::clearCurrent() {
  Mesh::clearCurrent();
  _vertBoneIndices.clear();
}

} // namespace fd
