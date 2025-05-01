#pragma once
#ifndef SHAPE_H
#define SHAPE_H

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

class Program;

enum class ShapeType { SPHERE, CUBE, BUNNY, TEAPOT };
/**
 * A shape defined by a list of triangles
 * - posBuf should be of length 3*ntris
 * - norBuf should be of length 3*ntris (if normals are available)
 * - texBuf should be of length 2*ntris (if texture coords are available)
 * posBufID, norBufID, and texBufID are OpenGL buffer identifiers.
 */
class Shape {
public:
  Shape();
  virtual ~Shape();
  void loadMesh(const std::string &meshName);
  void fitToUnitBox();
  void init();
  void draw(const std::shared_ptr<Program> prog) const;
  float getMinY() const; // To flush to floor
  glm::vec3 getBaseCenter() const;
  void setType(ShapeType shapeType) { this->type = shapeType; };
  ShapeType getType() { return this->type; };
  // For cube instancing
  GLuint getVAO() const { return vao; }
  int getVertexCount() const { return posBuf.size() / 3; }
  unsigned getPosBufID() { return posBufID; };
  unsigned getNorBufID() { return norBufID; };
  unsigned getTexBufID() { return texBufID; };

private:
  unsigned int vao;
  std::vector<float> posBuf;
  std::vector<float> norBuf;
  std::vector<float> texBuf;
  unsigned posBufID;
  unsigned norBufID;
  unsigned texBufID;
  ShapeType type;
};

#endif
