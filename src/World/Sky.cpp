#include "Sky.hpp"

Skybox::Skybox() noexcept
{
	TextFormat::log("Full sky creation: ");

	CreateClouds();
	CreateSkybox();
	CreateStars();
	CreateSunAndMoon();
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

void Skybox::RenderSunAndMoon() const noexcept
{
	game.shader.UseShader(Shader::ShaderID::SunMoon);
	glBindVertexArray(m_sunMoonVAO);
	glDrawArrays(GL_POINTS, 0, 4);
}

void Skybox::CreateClouds() noexcept
{
	TextFormat::log("Clouds creation");

	m_cloudsVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);
	glEnableVertexAttribArray(1u);
	glVertexAttribDivisor(1u, 1u); // Instanced rendering

	// Cloud shape data
	constexpr float cloudVertices[24] = {
		0.0f, 1.0f, 2.5f,	// Front-top-left	0
		2.5f, 1.0f, 2.5f,	// Front-top-right	1
		0.0f, 0.0f, 2.5f,	// Front-bottom-left	2
		2.5f, 0.0f, 2.5f,	// Front-bottom-right	3
		2.5f, 0.0f, 0.0f,	// Back-bottom-right	4
		2.5f, 1.0f, 0.0f,	// Back-top-right	5
		0.0f, 1.0f, 0.0f,	// Back-top-left	6
		0.0f, 0.0f, 0.0f,	// Back-bottom-left	7
	};

	// Buffer for cloud vertices
	m_cloudsShapeVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(cloudVertices), cloudVertices, 0u);

	constexpr std::uint8_t cloudIndices[14] = {
		0u, 1u, 2u, 3u, 4u, 1u, 5u, 0u, 6u, 2u, 7u, 4u, 6u, 5u 
	};

	// Buffer for indexes into cloud vertices
	m_cloudsShapeEBO = OGL::CreateBuffer(GL_ELEMENT_ARRAY_BUFFER);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(cloudIndices), cloudIndices, 0u);
	
	// Setup instanced cloud buffer
	m_cloudsInstVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(1u, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Cloud data (vec3)
	float* cloudData = new float[AMOUNT_OF_CLOUDS * 3];

	// Random number generator for cloud positions and size
	std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	// Cloud generation
	for (std::size_t i = 0; i < AMOUNT_OF_CLOUDS; ++i) {
		constexpr float positionMultiplier = 4000.0f, 
			posHalf = positionMultiplier * 0.5f,
			sizeMultiplier = 60.0f;
		
		// Calculate random X, Z and size for each cloud
		const float x = (dist(gen) * positionMultiplier) - posHalf;
		const float z = (dist(gen) * positionMultiplier) - posHalf;
		const float s = fmaxf(dist(gen), 0.02f) * sizeMultiplier;

		// Create vec3 for cloud properties and add it to the cloud data
		const float newCloud[3] = { x, z, s };
		std::memcpy(cloudData + (i * 3), newCloud, sizeof(newCloud));
	}

	// Buffer the float data into the GPU (in instanced buffer)
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float[3]) * AMOUNT_OF_CLOUDS, cloudData, 0u);

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

	const float triDist = 0.8660254f; // ~sqrt(0.75)

	// Skybox vertex data
	const float skyboxVertices[27] = {
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

	// Indices for vertex data
	const std::uint8_t skyboxIndices[] = {
		0u, 1u, 3u, 3u, 1u, 4u, 3u, 4u, 6u, 6u, 4u, 7u,
		0u, 3u, 2u, 2u, 3u, 5u, 3u, 8u, 5u, 8u, 3u, 6u,
		2u, 4u, 1u, 4u, 2u, 5u, 4u, 5u, 7u, 7u, 5u, 8u
	};

	// Create skybox buffers
	m_skyboxVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);

	// Setup sky vertices buffer
	m_skyboxVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, 0u);

	// Setup sky vertices indexes
	m_skyboxEBO = OGL::CreateBuffer(GL_ELEMENT_ARRAY_BUFFER);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, 0u);
}

void Skybox::CreateStars() noexcept
{
	TextFormat::log("Stars creation");

	// Star buffers
	m_starsVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);

	// Setup star shader attribute (vec4)
	m_starsVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

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
	
	constexpr float phi = 3.883222f;
	constexpr float floatNumStars = static_cast<float>(AMOUNT_OF_STARS);

	// Random real number (0.0f - 1.0f) generator
	std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> dist(0.0f, floatNumStars);

	// Weighted randomness list
	constexpr float weightedColourList[] = { 
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		2.0f, 2.0f, 2.0f, 2.0f, 2.0f,
		3.0f
	};
	constexpr int maxColourIndex = sizeof(weightedColourList) / sizeof(int);

	// Star data array
	float* starsData = new float[AMOUNT_OF_STARS * 4];

	for (std::size_t n = 0; n < AMOUNT_OF_STARS; ++n) {
		const float i = dist(gen); // Randomly selected position
		const float u = phi * i; // Calculate 'u' value

		// Calculate the star's relative XYZ position
		const float y = 1.0f - (i / floatNumStars) * 2.0f,
			r = sqrt(1.0f - (y * y)),
			x = (cos(u) * r),
			z = (sin(u) * r);

		// Using bit manipulation to store position and colour in a single int 
		const float star[4] = { x, y, z, weightedColourList[n % maxColourIndex] };
		std::memcpy(starsData + (n * 4), star, sizeof(star));
	}

	// Buffer star int data into GPU
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(float[4]) * AMOUNT_OF_STARS, starsData, 0u);

	// Free memory used by stars data
	delete[] starsData;
}

void Skybox::CreateSunAndMoon() noexcept
{
	TextFormat::log("Sun and moon creation");

	// Sun and moon buffers
	m_sunMoonVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);

	m_sunMoonVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 4, GL_FLOAT, false, 0, nullptr);

	// Place sun and moon directly opposite each other with same positioning as stars as seen above
	// Both the sun and moon (for now) are two squares: a larger, dark one and a smaller, light one on top
	// The last float is used as an integer for indexing to individual data.

	// Positions at time 0
	const glm::vec4 sunMoonData[4] = {
		{ 0.0f,  1.0f, 0.0f, 0.0f }, // Large sun square
		{ 0.0f,  1.0f, 0.0f, 1.0f }, // Small sun square
	
		{ 0.0f, -1.0f, 0.0f, 2.0f }, // Large moon square
		{ 0.0f, -1.0f, 0.0f, 3.0f }, // Small moon square
	};

	// Buffer sun and moon data to array buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(sunMoonData), sunMoonData, GL_STATIC_DRAW);
}

Skybox::~Skybox() noexcept
{
	// Delete all sky-related buffers (VBOs, EBOs)
	const GLuint deleteBuffers[7] = {
		m_cloudsInstVBO,
		m_cloudsShapeVBO,
		m_cloudsShapeEBO,
		m_skyboxVBO,
		m_skyboxEBO,
		m_starsVBO,
		m_sunMoonVBO
	};

	glDeleteBuffers(sizeof(deleteBuffers) / sizeof(GLuint), deleteBuffers);

	// Delete all sky-related VAOs
	const GLuint deleteVAOs[4] = {
		m_cloudsVAO,
		m_skyboxVAO,
		m_starsVAO,
		m_sunMoonVAO
	};

	glDeleteVertexArrays(sizeof(deleteVAOs) / sizeof(GLuint), deleteVAOs);
}
