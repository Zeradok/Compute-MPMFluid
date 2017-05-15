#version 430 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable

#UNIFORMS



layout( std430, binding=1 ) buffer Particles {
    Particle particles[];
};

layout( std430, binding=2 ) buffer Nodes {
    Node nodes[];
};


layout(local_size_x = COMPUTESIZE, local_size_y = 1, local_size_z = 1) in;

// compute shader to update particles
void main() {
	uint i = gl_GlobalInvocationID.x;

	// thread block size may not be exact multiple of number of particles
	if (i >= NPARTICLES) return;

	// read particles from buffers
	Particle p;
	Node n;

	n = nodes[i];
	if (n->m > 0.0) {
		n->u /= n->m;
		n->v /= n->m;
	}

	// write new values
	//if (i%2 == 1)
	particles[i].x = 0.5f;
	particles[i].y = p.y / 20;
	//nodes[i] = v;

}
