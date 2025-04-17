#include "Sky.hpp"

WorldSky::WorldSky() noexcept
{
	TextFormat::log("Sky enter");

	CreateClouds();
	CreateSkybox();
	CreateStars();
}

void WorldSky::RenderClouds() const noexcept
{
	game.shader.UseShader(Shader::ShaderID::Cloud);
	glBindVertexArray(m_cloudsVAO);
	glDrawElementsInstanced(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_BYTE, nullptr, AMOUNT_OF_CLOUDS);
}

void WorldSky::RenderSky() const noexcept
{
	game.shader.UseShader(Shader::ShaderID::Sky);
	glBindVertexArray(m_skyboxVAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, nullptr);
}

void WorldSky::RenderStars() const noexcept
{
	game.shader.UseShader(Shader::ShaderID::Stars);
	glBindVertexArray(m_starsVAO);
	glDrawArrays(GL_POINTS, 0, AMOUNT_OF_STARS);
}

void WorldSky::CreateClouds() noexcept
{
	TextFormat::log("Sky clouds");

	m_cloudsVAO = OGL::CreateVAO8();
	glEnableVertexAttribArray(0u);
	glEnableVertexAttribArray(1u);
	glVertexAttribDivisor(1u, 1u); // Instanced rendering

	// Cloud shape data
	constexpr float vertices[24] = {
		0.0f, 1.0f, 2.5f,	// Front-top-left		0
		2.5f, 1.0f, 2.5f,	// Front-top-right		1
		0.0f, 0.0f, 2.5f,	// Front-bottom-left	2
		2.5f, 0.0f, 2.5f,	// Front-bottom-right	3
		2.5f, 0.0f, 0.0f,	// Back-bottom-right	4
		2.5f, 1.0f, 0.0f,	// Back-top-right		5
		0.0f, 1.0f, 0.0f,	// Back-top-left		6
		0.0f, 0.0f, 0.0f,	// Back-bottom-left		7
	}; 
	constexpr uint8_t indices[14] = { 0, 1, 2, 3, 4, 1, 5, 0, 6, 2, 7, 4, 6, 5 };

	OGL::CreateBuffer(GL_ARRAY_BUFFER); // Instanced vertex buffer
	glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, sizeof(float[3]), nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(vertices), vertices, 0); 

	OGL::CreateBuffer(GL_ELEMENT_ARRAY_BUFFER); // Cloud shape indices - avoid redefining vertices
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, 0);
	
	// Cloud generation
	OGL::CreateBuffer(GL_ARRAY_BUFFER); // Compressed cloud data buffer

	// Setup cloud shader attribute
	glVertexAttribIPointer(1u, 1, GL_UNSIGNED_INT, sizeof(int32_t), nullptr);

	uint32_t cloudData[AMOUNT_OF_CLOUDS]{}; // Cloud data compressed into int for memory saving

	// Random number generator algorithm for cloud positions and size
	std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	for (int i = 0; i < AMOUNT_OF_CLOUDS; ++i) {
		// X and Z are in the range 0 - 4095 (12 bits each)
		// Width is in the range 0 - 255 (8 bits), height is a certain fraction of this
		// Y position is a set amount with slight variation using gl_InstanceID

		// Using bit manipulation to store XZ position and size into a single uint
		constexpr float posMultiplier = 4096.0f - 0.5f;
		cloudData[i] = 
			static_cast<uint32_t>(dist(gen) * posMultiplier) +				// X position
			(static_cast<uint32_t>(dist(gen) * posMultiplier) << 12u) +	// Z position
			(static_cast<uint32_t>(dist(gen) * 128.0f) << 26u);			// Size
	}

	// Buffer the int data into the GPU
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(cloudData), cloudData, 0);
}

void WorldSky::CreateSkybox() noexcept
{
	TextFormat::log("Sky skybox");

	/* 
		The skybox should just consist of the 'horizon' fading into the
		chosen background colour as glClearColor is used for the rest of the sky
		to avoid having to do large amounts of fragment shader work due to the
		sky taking up a large proportion of the view. Vertex colours are automatically
		interpolated between each other so it can just be a hollow triangle 'tube' shape
		around the player for less vertex shader work and memory usage as it will look the same,
		whether it is made up of 1000 triangles or 10.
	*/

	// Optimal skybox shape
	static constexpr float skyboxVertices[27] = {
		-triDist,  -0.1f, -0.5f, // 0
		 triDist,  -0.1f, -0.5f, // 1	Lower triangle
		 0.0f,	   -0.1f,  1.0f, // 2

		-triDist,   0.0f, -0.5f, // 3
		 triDist,   0.0f, -0.5f, // 4	Middle triangle
		 0.0f,	    0.0f,  1.0f, // 5

		-triDist,   0.1f, -0.5f, // 6
		 triDist,   0.1f, -0.5f, // 7	Upper triangle
		 0.0f,	    0.1f,  1.0f, // 8
	};
	static constexpr uint8_t skyboxIndices[] = {
		0, 1, 3, 3, 1, 4, 3, 4, 6, 6, 4, 7,
		0, 3, 2, 2, 3, 5, 3 ,8, 5, 8, 3, 6,
		2, 4, 1, 4, 2, 5, 4, 5, 7, 7, 5, 8
	};

	// Create skybox buffers
	m_skyboxVAO = OGL::CreateVAO8();
	OGL::CreateBuffer(GL_ELEMENT_ARRAY_BUFFER);
	OGL::CreateBuffer(GL_ARRAY_BUFFER);

	// Setup sky shader attributes
	glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, sizeof(float[3]), nullptr);
	glEnableVertexAttribArray(0u);

	// Save skybox data into buffers
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, 0);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, 0);
}

void WorldSky::CreateStars() noexcept
{
	TextFormat::log("Sky stars");

	// Star buffers
	m_starsVAO = OGL::CreateVAO8();
	OGL::CreateBuffer(GL_ARRAY_BUFFER);

	// Setup star shader attribute (compressed all into 1 int only)
	glVertexAttribIPointer(0u, 1, GL_INT, sizeof(int32_t), nullptr);
	glEnableVertexAttribArray(0u);

	// Weighted randomness list
	constexpr uint8_t randcol[10] = { 0, 0, 0, 0, 0, 0, 0, 1, 2, 3 };

	// Compressed star data array
	int32_t* starsData = new int32_t[AMOUNT_OF_STARS];

	/* 
		Using Fibonnaci sphere algorithm gives more evenly distributed
		sphere positions rather than a large proportion being at the top
		and bottom. Random samples are taken so they are not predictable 
		in their size and/or position for a more realistic night sky.

		(constant) phi = pi * (sqrt(5) - 1)
		i = [0, max)

		u = phi * i

		y = 1 - (i / max) * 2
		r = sqrt(1 - (y * y))

		x = cos(u) * r
		z = sin(u) * r
	*/
	
	constexpr float phi = Math::PI_FLT * (static_cast<float>(Math::sqrt(5.0)) - 1.0f);
	constexpr float floatAmount = static_cast<float>(AMOUNT_OF_STARS);

	// Random number generator (avoid use of rand() due to predictability)
	std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> dist(0.0f, floatAmount);

	for (int n = 0; n < AMOUNT_OF_STARS; ++n) {
		const float i = dist(gen); // Randomly selected position
		const float u = phi * i;

		// X and Z are in the range -1.0f to 1.0f so they need to be incremented and
		// multiplied by 511.5 to change to the range 0 - 1023 (10 bits each)

		const float y = 1.0f - (i / floatAmount) * 2.0f,
			r = sqrt(1.0f - (y * y)),
			x = (cos(u) * r) + 1.0f,
			z = (sin(u) * r) + 1.0f;

		// Using bit manipulation to store position and colour in a single int 
		starsData[n] = static_cast<int32_t>(x * 511.5f) + 
			(static_cast<int32_t>((y + 1.0f) * 511.5f) << 10) +
			(static_cast<int32_t>(z * 511.5f) << 20) + 
			(randcol[n % 10] << 30);
	}

	// Buffer star int data into GPU
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(int32_t) * WorldSky::AMOUNT_OF_STARS, starsData, 0);
	// Free memory used by stars
	delete[] starsData;
}
