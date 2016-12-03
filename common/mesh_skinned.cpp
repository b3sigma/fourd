#include <memory>
#include "camera.h"
#include "mesh_skinned.h"

namespace fd {

void MeshSkinned::buildAxisSignPost() {
  float segmentLength = 10.0f;

  {
    Bone bone;
    bone._poseRelative.storeIdentity();
    bone._parentIndex = Bone::InvalidParentIndex;
    _bones.push_back(bone);
  }

  for(int up = 0; up < 4; up++) {
    Bone bone;
    bone._poseRelative.rotation.storeIdentity();
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  int branchIndex = (int)_bones.size() - 1;

  {
    Bone bone;
    bone._poseRelative.rotation.storeRotation((float)PI / 2.0f, Camera::UP, Camera::RIGHT);
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = branchIndex;
    _bones.push_back(bone);
  }

  for(int right = 0; right < 2; right++) {
    Bone bone;
    bone._poseRelative.rotation.storeIdentity();
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._poseRelative.rotation.storeRotation((float)PI / 2.0f, Camera::UP, Camera::FORWARD);
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = branchIndex;
    _bones.push_back(bone);
  }

  for(int forward = 0; forward < 2; forward++) {
    Bone bone;
    bone._poseRelative.rotation.storeIdentity();
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._poseRelative.rotation.storeRotation((float)PI / 2.0f, Camera::UP, Camera::INSIDE);
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = branchIndex;
    _bones.push_back(bone);
  }

  for(int in = 0; in < 2; in++) {
    Bone bone;
    bone._poseRelative.rotation.storeIdentity();
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  fillBonesWithTesseracts(segmentLength);
}

void MeshSkinned::buildTilt() {
  float segmentLength = 10.0f;

  {
    Bone bone;
    bone._poseRelative.storeIdentity();
    bone._parentIndex = Bone::InvalidParentIndex;
    _bones.push_back(bone);
  }

  for(int up = 0; up < 4; up++) {
    Bone bone;
    bone._poseRelative.rotation.storeRotation((float)PI / 16.0f, Camera::UP, Camera::RIGHT); //storeIdentity();
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  fillBonesWithTesseracts(segmentLength);
}


void MeshSkinned::buildCactusDancer() {
  // usually we might want the root elsewhere but in this case put it on the ground to make things easier to debug initially
  float segmentLength = 10.0f;
  
  {
    Bone bone;
    bone._poseRelative.storeIdentity();
    bone._parentIndex = Bone::InvalidParentIndex;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._poseRelative.rotation.storeRotation((float)PI / 16.0f, Camera::UP, Camera::RIGHT);
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._poseRelative.rotation.storeRotation((float)PI / 16.0f, Camera::UP, Camera::FORWARD);
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._poseRelative.rotation.storeRotation((float)PI / 16.0f, Camera::UP, Camera::INSIDE);
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  {

    Bone bone;
    bone._poseRelative.rotation.storeIdentity();
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }
  int branchBone = (int)_bones.size() - 1;

  {
    Bone bone;
    bone._poseRelative.rotation.storeRotation((float)PI / 2.0f, Camera::UP, Camera::RIGHT);
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = branchBone;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._poseRelative.rotation.storeIdentity();
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._poseRelative.rotation.storeRotation(-(float)PI / 2.0f, Camera::UP, Camera::RIGHT);
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = branchBone;
    _bones.push_back(bone);
  }

  {
    Bone bone;
    bone._poseRelative.rotation.storeIdentity();
    bone._poseRelative.position.set(0.0f, segmentLength, 0.0f, 0.0f);
    bone._parentIndex = (int)_bones.size() - 1;
    _bones.push_back(bone);
  }

  fillBonesWithTesseracts(segmentLength);
}

void MeshSkinned::fillBonesWithTesseracts(float segmentLength) {
  updateFullPoses();

  std::unique_ptr<Mesh> mesh(new Mesh());
  mesh->buildTesseract(segmentLength);

  _vertBoneIndices.resize(0);
  _vertBoneIndices.reserve(_bones.size() * mesh->_verts.size());
  for(int bone = 0; bone < (int)_bones.size(); bone++) {
    merge(*mesh);

    for(auto vert : mesh->_verts) {
      _vertBoneIndices.push_back(bone);
    }
  }

}

// recursively updates parents first,
// doing a memoization with dirtyCounter that is incremented each frame by caller
void MeshSkinned::updateBoneRecursive(int index, char dirtyCounter) {
  assert(index >= 0 && index < (int)_bones.size());
  MeshSkinned::Bone& bone = _bones[index];
  if(bone._dirtyCounter == dirtyCounter)
    return;

  if(bone._parentIndex == MeshSkinned::Bone::InvalidParentIndex) {
    _bonePositions[index] = bone._poseRelative.position;
    _boneRotations[index] = bone._poseRelative.rotation;
    bone._dirtyCounter = dirtyCounter;
    return;
  }

  updateBoneRecursive(bone._parentIndex, dirtyCounter);
  MeshSkinned::Bone& parentBone = _bones[bone._parentIndex];

  // transpose makes it terrible.
  _bonePositions[index] = _boneRotations[bone._parentIndex].transpose().transform(bone._poseRelative.position) + _bonePositions[bone._parentIndex];
  _boneRotations[index] = bone._poseRelative.rotation * _boneRotations[bone._parentIndex];
  bone._dirtyCounter = dirtyCounter;
}

void MeshSkinned::updateFullPoses() {
  if(_bonePositions.size() != _bones.size()) {
    _bonePositions.resize(_bones.size());
  }
  if(_boneRotations.size() != _bones.size()) {
    _boneRotations.resize(_bones.size());
  }

  static int dirtyCounter = 0;
  dirtyCounter++;
  for(int i = 0; i < (int)_bones.size(); i++) {
    updateBoneRecursive(i, dirtyCounter);
  }
}

void MeshSkinned::clearCurrent() {
  Mesh::clearCurrent();
  _vertBoneIndices.clear();
}

} // namespace fd
