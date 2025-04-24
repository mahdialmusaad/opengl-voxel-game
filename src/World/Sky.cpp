#include "Sky.hpp"

Skybox::Skybox() noexcept
{
	TextFormat::log("Full sky creation: ");

	CreateClouds();
	CreateSkybox();
	CreateStars();
}

void Skybox::RenderClouds() const noexcept
{
	game.shader.UseShader(Shader::ShaderID::Clouds);
	glBindVertexArray(m_cloudsVAO);
	glDrawElementsInstanced(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_BYTE, nullptr, AMOUNT_OF_CLOUDS);
}

void Skybox::RenderSky() const noexcept
{
	game.shader.UseShader(Shader::ShaderID::Sky);
	glBindVertexArray(m_skyboxVAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, nullptr);
}

void Skybox::RenderStars() const noexcept
{
	game.shader.UseShader(Shader::ShaderID::Stars);
	glBindVertexArray(m_starsVAO);
	glDrawArrays(GL_POINTS, 0, AMOUNT_OF_STARS);
}

void Skybox::CreateClouds() noexcept
{
	TextFormat::log("Clouds creation");

	m_cloudsVAO = OGL::CreateVAO8();
	glEnableVertexAttribArray(0u);
	glEnableVertexAttribArray(1u);
	glVertexAttribDivisor(0u, 1u); // Instanced rendering

	// Cloud shape data
	constexpr float cloudVertices[24] = {
		0.0f, 1.0f, 2.5f,	// Front-top-left		0
		2.5f, 1.0f, 2.5f,	// Front-top-right		1
		0.0f, 0.0f, 2.5f,	// Front-bottom-left	2
		2.5f, 0.0f, 2.5f,	// Front-bottom-right	3
		2.5f, 0.0f, 0.0f,	// Back-bottom-right	4
		2.5f, 1.0f, 0.0f,	// Back-top-right		5
		0.0f, 1.0f, 0.0f,	// Back-top-left		6
		0.0f, 0.0f, 0.0f,	// Back-bottom-left		7
	};

	// Buffer for cloud vertices
	m_cloudsShapeVBO = OGL::CreateBuffer8(GL_ARRAY_BUFFER);
	glVertexAttribPointer(1u, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(cloudVertices), cloudVertices, 0u);

	constexpr std::uint8_t cloudIndices[14] = {
		0u, 1u, 2u, 3u, 4u, 1u, 5u, 0u, 6u, 2u, 7u, 4u, 6u, 5u 
	};

	// Buffer for indexes into cloud vertices
	OGL::CreateBuffer(GL_ELEMENT_ARRAY_BUFFER);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(cloudIndices), cloudIndices, 0u);
	
	// Setup instanced cloud buffer
	m_cloudsInstVBO = OGL::CreateBuffer8(GL_ARRAY_BUFFER);
	glVertexAttribIPointer(0u, 1, GL_UNSIGNED_INT, 0, nullptr);

	// Compressed cloud data
	std::uint32_t* cloudData = new std::uint32_t[AMOUNT_OF_CLOUDS];

	// Random number generator for cloud positions and size
	std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	// Cloud generation
	for (std::size_t i = 0; i < AMOUNT_OF_CLOUDS; ++i) {
		// X and Z are in the range 0 - 4095 (12 bits each)
		// Size is in the range 0 - 255 (8 bits), height is a certain fraction of this
		// Y position is a set amount with slight variation using gl_InstanceID

		// Using bit manipulation to store XZ position and size into a single uint
		cloudData[i] = 
			static_cast<std::uint32_t>(dist(gen) * 4095.0f) +						// X position
			(static_cast<std::uint32_t>(dist(gen) * 4095.0f) << 12u) +				// Z position
			(static_cast<std::uint32_t>(fmaxf(dist(gen), 0.01f) * 255.0f) << 24u);	// Size
	}

	// Buffer the compressed integer data into the GPU - use instanced buffer
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(std::uint32_t) * AMOUNT_OF_CLOUDS, cloudData, 0u);

	// Free memory used by cloud data
	delete[] cloudData;
}

void Skybox::CreateSkybox() noexcept
{
	TextFormat::log("Skybox creation");

	/* 
		The skybox should just consist of the 'horizon' fading into the
		chosen background colour as glClearColor is used for the rest of the sky
		to avoid having to do large amounts of fragment shader work due to the
		sky taking up a large proportion of the view. Vertex colours are automatically
		interpolated between each other so it can just be a hollow triangle 'tube' shape
		around the player for less vertex shader work and memory usage as it will look the same,
		whether it is made up of 1000 triangles or 10.
	*/

	// Optimal skybox data
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

	static constexpr std::uint8_t skyboxIndices[] = {
		0u, 1u, 3u, 3u, 1u, 4u, 3u, 4u, 6u, 6u, 4u, 7u,
		0u, 3u, 2u, 2u, 3u, 5u, 3u, 8u, 5u, 8u, 3u, 6u,
		2u, 4u, 1u, 4u, 2u, 5u, 4u, 5u, 7u, 7u, 5u, 8u
	};

	// Create skybox buffers
	m_skyboxVAO = OGL::CreateVAO8();
	glEnableVertexAttribArray(0u);

	// Setup sky vertices buffer
	OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, 0u);

	// Setup sky vertices indexes
	OGL::CreateBuffer(GL_ELEMENT_ARRAY_BUFFER);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, 0u);
}

void Skybox::CreateStars() noexcept
{
	TextFormat::log("Stars creation");

	// Star buffers
	m_starsVAO = OGL::CreateVAO8();
	glEnableVertexAttribArray(0u);

	// Setup star shader attribute (compressed all into 1 int only)
	OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribIPointer(0u, 1, GL_UNSIGNED_INT, 0, nullptr);

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

	// Random number generator (avoid use of rand() due to predictability and bad results in general)
	std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> dist(0.0f, floatAmount);

	// Weighted randomness list
	constexpr int randcol[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3 };
	constexpr int randcolmax = sizeof(randcol) / sizeof(int);

	// Compressed star data array
	std::uint32_t* starsData = new std::uint32_t[AMOUNT_OF_STARS];

	for (std::size_t n = 0; n < AMOUNT_OF_STARS; ++n) {
		const float i = dist(gen); // Randomly selected position
		const float u = phi * i;

		// To store the floating point positions, the XYZ position can be multiplied to
		// the maximum value able to be held in the bits allocated for each (10 in this case = 1023),
		// so they can be multiplied by half of that (511.5) to get their positions back (1.0f to -1.0f)

		const float y = 1.0f - (i / floatAmount) * 2.0f,
			r = sqrt(1.0f - (y * y)),
			x = (cos(u) * r) + 1.0f, // trig(u) * r gives value -1.0f to 1.0f, so add 1.0f to make it positive
			z = (sin(u) * r) + 1.0f;

		// Using bit manipulation to store position and colour in a single int 
		starsData[n] = static_cast<std::uint32_t>(x * 511.5f) + 
			static_cast<std::uint32_t>(static_cast<int>((y + 1.0f) * 511.5f) << 10) +
			static_cast<std::uint32_t>(static_cast<int>(z * 511.5f) << 20) + 
			static_cast<std::uint32_t>(randcol[n % randcolmax] << 30);
	}

	// Buffer star int data into GPU
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(std::uint32_t) * AMOUNT_OF_STARS, starsData, 0u);

	// Free memory used by stars data
	delete[] starsData;
}

Skybox::~Skybox() noexcept
{
	// The stars and skybox only use 1 unique buffer per type, so they can be easily retrieved using
	// glGetIntegerv(...). However, both the cloud array buffers need to be explicitly saved.
	
	GLint skyVBO, skyEBO, starsVBO;

	glBindVertexArray(m_skyboxVAO);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &skyVBO);
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER, &skyEBO);

	glBindVertexArray(m_starsVAO);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &starsVBO);

	// Delete buffers in array
	const GLuint deleteBuffers[] = {
		static_cast<GLuint>(skyVBO),
		static_cast<GLuint>(skyEBO),
		static_cast<GLuint>(starsVBO),
		static_cast<GLuint>(m_cloudsInstVBO),
		static_cast<GLuint>(m_cloudsShapeVBO)
	};
	glDeleteBuffers(sizeof(deleteBuffers) / sizeof(GLuint), deleteBuffers);

	// Delete all three VAOs
	const GLuint deleteVAOs[] = {
		static_cast<GLuint>(m_skyboxVAO),
		static_cast<GLuint>(m_cloudsVAO),
		static_cast<GLuint>(m_starsVAO),
	};
	glDeleteVertexArrays(sizeof(deleteVAOs) / sizeof(GLuint), deleteVAOs);
}
