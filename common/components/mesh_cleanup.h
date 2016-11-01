#pragma once

#include <algorithm>

#include "../component.h"
#include "../mesh.h"

namespace fd {

// Deletes the dereferenced pointer and sets the pointer to null on it's own destruction.
// Useful for deleting something on another class that really ought to be externally memory managed.
// Could do a refcounting thing also, but if you need something more complex than this, consider a mesh manager for efficiency instead.
class MeshCleanupComponent : public Component {
protected:
  int _debugDeathCounter;
  Mesh** _mesh; // will cleanup the 

public:
  MeshCleanupComponent(Mesh** mesh)
      : _mesh(mesh)
  {
  }

  virtual ~MeshCleanupComponent() {
    OnIdempotentDestruction();
  }

  virtual void OnIdempotentDestruction() {
    if(_mesh) {
      delete *_mesh;
      *_mesh = NULL;
      _mesh = NULL;
    }
  }
};

} // namespace fd
