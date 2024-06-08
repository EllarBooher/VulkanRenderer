#include "debuglines.hpp"

void DebugLines::clear()
{
    vertices->clearStaged();
    indices->clearStaged();
}

void DebugLines::push(
    glm::vec3 const start
    , glm::vec3 const end
)
{
    Vertex const startVertex{
        .position = start,
        .uv_x = 0.0,
        .normal = glm::vec3(0.0),
        .uv_y = 0.0,
        .color = glm::vec4(1.0,0.0,0.0,1.0),
    };
    Vertex const endVertex{
        .position = end,
        .uv_x = 1.0,
        .normal = glm::vec3(0.0),
        .uv_y = 0.0,
        .color = glm::vec4(0.0,0.0,1.0,1.0),
    };

    uint32_t const index{ static_cast<uint32_t>(indices->stagedSize()) };

    vertices->push(std::initializer_list<Vertex>{ startVertex, endVertex });
    indices->push(std::initializer_list<uint32_t>{ index, index + 1 });
}

void DebugLines::pushQuad(
    glm::vec3 const a
    , glm::vec3 const b
    , glm::vec3 const c
    , glm::vec3 const d
)
{
    push(a, b);
    push(b, c);
    push(c, d);
    push(d, a);
}

void DebugLines::pushRectangleAxes(
    glm::vec3 const center
    , glm::vec3 const extentA
    , glm::vec3 const extentB
)
{
    pushQuad(
        center + extentA + extentB
        , center + extentA - extentB
        , center - extentA - extentB
        , center - extentA + extentB
    );
}

void DebugLines::pushRectangleOriented(
    glm::vec3 const center
    , glm::quat const orientation
    , glm::vec2 const extents
)
{
    glm::vec3 const scale{extents.x, 1.0F, extents.y};

    glm::vec3 const right{ orientation * (scale * geometry::right) };
    glm::vec3 const forward{ orientation * (scale * geometry::forward) };

    pushRectangleAxes(center, right, forward);
}

void DebugLines::pushBox(
    glm::vec3 const center
    , glm::quat const orientation
    , glm::vec3 const extents
)
{
    glm::vec3 const right{ orientation * (extents * geometry::right) };
    glm::vec3 const forward{ orientation * (extents * geometry::forward) };
    glm::vec3 const up{ orientation * (extents * geometry::up) };

    pushRectangleAxes(center - up, right, forward);
    pushRectangleAxes(center + up, right, forward);

    pushRectangleAxes(center - right, forward, up);
    pushRectangleAxes(center + right, forward, up);

    pushRectangleAxes(center - forward, up, right);
    pushRectangleAxes(center + forward, up, right);
}

void DebugLines::recordCopy(
    VkCommandBuffer const cmd
    , VmaAllocator const allocator
)
{
    vertices->recordCopyToDevice(cmd, allocator);
    indices->recordCopyToDevice(cmd, allocator);
}

void DebugLines::cleanup(
    VkDevice const device
    , VmaAllocator const allocator
)
{
    pipeline->cleanup(device);
    pipeline.reset();
    vertices.reset();
    indices.reset();
}
