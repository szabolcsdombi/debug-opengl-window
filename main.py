import zengl

import core

core.init(debug=True)
ctx = zengl.context()

image = ctx.image(core.window_size, 'rgba8unorm', samples=4)
image.clear_value = (1.0, 1.0, 1.0, 1.0)

pipeline = ctx.pipeline(
    vertex_shader='''
        #version 330 core

        out vec3 v_color;

        vec2 positions[3] = vec2[](
            vec2(0.0, 0.8),
            vec2(-0.6, -0.8),
            vec2(0.6, -0.8)
        );

        vec3 colors[3] = vec3[](
            vec3(1.0, 0.0, 0.0),
            vec3(0.0, 1.0, 0.0),
            vec3(0.0, 0.0, 1.0)
        );

        void main() {
            gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
            v_color = colors[gl_VertexID];
        }
    ''',
    fragment_shader='''
        #version 330 core

        in vec3 v_color;

        layout (location = 0) out vec4 out_color;

        void main() {
            out_color = vec4(v_color, 1.0);
        }
    ''',
    framebuffer=[image],
    topology='triangles',
    vertex_count=3,
)

while core.update():
    image.clear()
    pipeline.render()
    image.blit()
