#include "StaticMesh.h"

#include <glad/gl.h>

#include "glm/glm.hpp"
#include "glm/geometric.hpp"

namespace OM3D {
    extern bool audit_bindings_before_draw;

    StaticMesh::StaticMesh(const MeshData& data) :
        _vertex_buffer(data.vertices),
        _index_buffer(data.indices) {
        if (data.vertices.empty()) {
            _bounding_sphere.origin = glm::vec3(INFINITY);
            _bounding_sphere.radius = 0.f;
        }
        else {
            glm::vec3 min = data.vertices[0].position;
            glm::vec3 max = data.vertices[0].position;

            for (auto vertex : data.vertices) {
                min = {
                    glm::min(min.x, vertex.position.x),
                    glm::min(min.y, vertex.position.y),
                    glm::min(min.z, vertex.position.z),
                };
                max = {
                    glm::max(max.x, vertex.position.x),
                    glm::max(max.y, vertex.position.y),
                    glm::max(max.z, vertex.position.z),
                };
            }
            _bounding_sphere.origin = (min + max) / 2.f;
            _bounding_sphere.radius = glm::length(max - min) / 2.f;
        }
    }

    bool StaticMesh::collide(const Camera& camera, const glm::mat4 &transform) const
    {
        if (_bounding_sphere.radius <= 0.f)
            return false;

        const float sx = glm::length(glm::vec3(transform[0]));
        const float sy = glm::length(glm::vec3(transform[1]));
        const float sz = glm::length(glm::vec3(transform[2]));

        const float world_radius = _bounding_sphere.radius * std::max(std::max(sx, sy), sz);
        const auto world_origin = glm::vec3(transform * glm::vec4(_bounding_sphere.origin, 1.));

        auto in_plane = [world_radius](const glm::vec3& n, const glm::vec3& p) {
            return glm::dot(glm::normalize(n), p) > - world_radius;
        };

        auto [near_normal, top_normal, bottom_normal, right_normal, left_normal] = camera.build_frustum();


        if (camera.is_orthographic()) {

            // Get camera matrices
            const glm::mat4 view = camera.view_matrix();
            const glm::mat4 proj = camera.projection_matrix();
            const glm::mat4 invView = glm::inverse(view);

            const glm::vec3 camPos   = glm::vec3(invView[3]);
            const glm::vec3 camRight = glm::normalize(glm::vec3(invView[0]));
            const glm::vec3 camUp    = glm::normalize(glm::vec3(invView[1]));
            const glm::vec3 camFwd   = glm::normalize(glm::vec3(invView[2]));

            const float left   = -(proj[3][0] + 1.0f) / proj[0][0];
            const float right  =  (1.0f - proj[3][0]) / proj[0][0];
            const float bottom = -(proj[3][1] + 1.0f) / proj[1][1];
            const float top    =  (1.0f - proj[3][1]) / proj[1][1];
            const float nearZ  =  (proj[3][2] + 1.0f) / proj[2][2];

            const glm::vec3 c_left   = world_origin - (camPos + camRight * left);
            const glm::vec3 c_right  = world_origin - (camPos + camRight * right);
            const glm::vec3 c_top    = world_origin - (camPos + camUp    * top);
            const glm::vec3 c_bottom = world_origin - (camPos + camUp    * bottom);
            const glm::vec3 c_near   = world_origin - (camPos + camFwd   * nearZ);

            return in_plane(bottom_normal, c_bottom)
                && in_plane(left_normal,   c_left)
                && in_plane(right_normal,  c_right)
                && in_plane(top_normal,    c_top)
                && in_plane(near_normal,   c_near);
        } else {
            const auto c = world_origin - camera.position();
            return in_plane(bottom_normal, c)
                && in_plane(left_normal, c)
                && in_plane(right_normal, c)
                && in_plane(top_normal, c)
                && in_plane(near_normal, c);
        }
    }

    BoundingSphere StaticMesh::get_bounding_sphere() const {
        return _bounding_sphere;
    }

    void StaticMesh::draw() const {
    _vertex_buffer.bind(BufferUsage::Attribute);
    _index_buffer.bind(BufferUsage::Index);

    // Vertex position
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), nullptr);
    // Vertex normal
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));
    // Vertex uv
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(6 * sizeof(float)));
    // Tangent / bitangent sign
    glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(8 * sizeof(float)));
    // Vertex color
    glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(12 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    if(audit_bindings_before_draw) {
        audit_bindings();
    }

    glDrawElements(GL_TRIANGLES, int(_index_buffer.element_count()), GL_UNSIGNED_INT, nullptr);
}

}
