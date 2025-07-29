#include "Sky.hpp"

Skybox::Skybox() noexcept
{
	CreateClouds();
	CreateSkybox();
	CreateStars();
	CreatePlanets();
}

void Skybox::RenderClouds() const noexcept
{
	shader.UseProgram(shader.programs.clouds);
	glBindVertexArray(m_cloudsVAO);
	glDrawElementsInstanced(GL_TRIANGLE_STRIP, static_cast<GLsizei>(14), GL_UNSIGNED_BYTE, nullptr, numClouds);
}

void Skybox::RenderSky() const noexcept
{
	shader.UseProgram(shader.programs.sky);
	glBindVertexArray(m_skyboxVAO);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(36), GL_UNSIGNED_BYTE, nullptr);
}

void Skybox::RenderStars() const noexcept
{
	shader.UseProgram(shader.programs.stars);
	glBindVertexArray(m_starsVAO);
	glDrawArrays(GL_POINTS, 0, numStars);
}

void Skybox::RenderPlanets() const noexcept
{
	shader.UseProgram(shader.programs.planets);
	glBindVertexArray(m_planetsVAO);
	glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_BYTE, nullptr);
}

void Skybox::CreateClouds() noexcept
{
	m_cloudsVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);
	glEnableVertexAttribArray(1u);
	glVertexAttribDivisor(1u, 1u); // Instanced rendering

	// Cloud shape data
	const float cloudVertices[24] =  {
		0.0f, 1.0f, 2.5f,  // Front-top-left     0
		2.5f, 1.0f, 2.5f,  // Front-top-right    1
		0.0f, 0.0f, 2.5f,  // Front-bottom-left  2
		2.5f, 0.0f, 2.5f,  // Front-bottom-right 3
		2.5f, 0.0f, 0.0f,  // Back-bottom-right  4
		2.5f, 1.0f, 0.0f,  // Back-top-right     5
		0.0f, 1.0f, 0.0f,  // Back-top-left      6
		0.0f, 0.0f, 0.0f,  // Back-bottom-left   7
	};

	// Buffer for cloud vertices
	m_cloudsShapeVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(cloudVertices)), cloudVertices, GLbitfield{});
	
	// Buffer for indexes into cloud vertices
	const std::uint8_t cloudIndices[14] = { 0u, 1u, 2u, 3u, 4u, 1u, 5u, 0u, 6u, 2u, 7u, 4u, 6u, 5u };
	m_cloudsShapeEBO = OGL::CreateBuffer(GL_ELEMENT_ARRAY_BUFFER);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(cloudIndices)), cloudIndices, GLbitfield{});
	
	// Setup instanced cloud buffer
	m_cloudsInstVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(1u, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	
	const int numClouds = 300;
	glm::vec3 *cloudData = new glm::vec3[numClouds]; // Cloud data (vec3)

	// Random number generator for cloud positions and size
	std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	// Cloud generation
	for (int i = 0; i < numClouds; ++i) {
		const float positionMultiplier = 4000.0f, 
		    posHalf = positionMultiplier * 0.5f,
		    sizeMultiplier = 60.0f;
		
		// Calculate random X, Z and size for each cloud
		const float x = (dist(gen) * positionMultiplier) - posHalf;
		const float z = (dist(gen) * positionMultiplier) - posHalf;
		const float s = fmaxf(dist(gen), 0.02f) * sizeMultiplier;

		// Create vec3 for cloud properties and add it to the cloud data
		const glm::vec3 newCloudData = glm::vec3(x, z, s);
		std::memcpy(cloudData + i, &newCloudData, sizeof(newCloudData));
	}

	// Buffer the float data into the GPU (in instanced buffer)
	glBufferStorage(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(glm::vec3) * static_cast<std::size_t>(numClouds)), cloudData, GLbitfield{});

	// Free memory used by cloud data
	delete[] cloudData;
}

void Skybox::CreateSkybox() noexcept
{
	// Create skybox buffers
	m_skyboxVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);

	/* 
	    Since the sky is just a single colour (glClearColor), a specific 
	    part of it can have triangles covering that changes colour and fade 
	    to the 'cleared' sky colour to imitate a sunrise/set. This saves on 
	    fragment shader calculations for the rest of the sky.
	*/

	// Skybox vertex data
	const float triDist = static_cast<float>(Math::sqrt(0.75));
	const float skyboxVertices[27] = {
		-triDist, -0.1f, -0.5f, // 0
		 triDist, -0.1f, -0.5f, // 1  Lower triangle
		 0.0f,    -0.1f,  1.0f, // 2

		-triDist,  0.0f, -0.5f, // 3
		 triDist,  0.0f, -0.5f, // 4  Middle triangle
		 0.0f,     0.0f,  1.0f, // 5

		-triDist,  0.1f, -0.5f, // 6
		 triDist,  0.1f, -0.5f, // 7  Upper triangle
		 0.0f,     0.1f,  1.0f, // 8
	};

	// Setup sky vertices buffer
	m_skyboxVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(skyboxVertices)), skyboxVertices, GLbitfield{});

	// Indices for vertex data
	const std::uint8_t skyboxIndices[] = {
		0u, 1u, 3u, 3u, 1u, 4u, 3u, 4u, 6u, 6u, 4u, 7u,
		0u, 3u, 2u, 2u, 3u, 5u, 3u, 8u, 5u, 8u, 3u, 6u,
		2u, 4u, 1u, 4u, 2u, 5u, 4u, 5u, 7u, 7u, 5u, 8u
	};

	// Setup sky vertices indexes
	m_skyboxEBO = OGL::CreateBuffer(GL_ELEMENT_ARRAY_BUFFER);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(skyboxIndices)), skyboxIndices, GLbitfield{});
}

void Skybox::CreateStars() noexcept
{
	// Star VAO
	m_starsVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);
	
	// Since a massive number of stars are being created, it is more efficient to
	// split the work amongst threads (2 for now) instead of using a single-threaded version.

	glm::vec4* starsData = new glm::vec4[numStars]; // Star data array (vec4 each)
	const int max = static_cast<int>(numStars), halfmax = max / 2;

	// Create threads to work at specific sections of the array
	std::thread starThread1 = std::thread(CreateStarsImpl, this, starsData, 0, halfmax);
	std::thread starThread2 = std::thread(CreateStarsImpl, this, starsData, halfmax, max);
	// Wait for the threads to finish
	starThread1.join();
	starThread2.join();

	// Create buffer for unique positions and buffer it
	m_starsVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(glm::vec4) * static_cast<std::size_t>(numStars)), starsData, GLbitfield{});
	 
	delete[] starsData; // Free memory used by stars data
}

void Skybox::CreateStarsImpl(glm::vec4 *data, int index, int end) noexcept
{
	
	/* 
	   Using the Fibonnaci sphere algorithm gives more evenly distributed positions
	   on a sphere rather than a large proportion being at the top and bottom. 
	   Random samples are taken so the stars do not appear in a pattern.

	   phi = pi * (sqrt(5) - 1)
	   i = [0, max)

	   u = phi * i

	   y = 1 - (i / max) * 2
	   r = sqrt(1 - (y * y))

	   x = cos(u) * r
	   z = sin(u) * r
	*/

	const float phi = glm::pi<float>() * (static_cast<float>(Math::sqrt(5.0)) - 1.0f);
	const float floatNumStars = static_cast<float>(numStars);

	// Random real number (0.0f - 1.0f) generator
	std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	for (; index < end; ++index) {
		const float i = dist(gen); // Randomly selected position
		const float u = phi * (i * floatNumStars); // Calculate 'u' value
		
		// Calculate the star's position and store it in given array with 'size' as [0-2]
		const float y = 1.0f - (i * 2.0f), r = glm::sqrt(1.0f - (y * y));
		data[index] = { glm::cos(u) * r, y, glm::sin(u) * r, dist(gen) * 2.0f };
	}
}

void Skybox::CreatePlanets() noexcept
{
	// Planets buffers
	m_planetsVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);

	// Place sun and moon directly opposite each other with same positioning as stars as seen above
	// Both the sun and moon (for now) are two squares: a larger, dark one and a smaller, lighter one on top;
	// Data is laid out as such: { Xpos, Zpos, Rcol, Bcol }

	m_planetsVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 4, GL_FLOAT, false, 0, nullptr); // Using vec4

	// Positions at 0 seconds of in-game day time
	const float sunSizeBig = 0.2f, sunSizeLow = sunSizeBig * 0.8f, moonSizeBig = 0.05f, moonSizeLow = moonSizeBig * 0.8f;
	const glm::vec4 planetsData[] = {
		             // Large sun square
		{ -sunSizeBig,  -sunSizeBig,   0.92f, 0.43f }, // TL 0
		{  sunSizeBig,  -sunSizeBig,   0.92f, 0.43f }, // BL 1
		{ -sunSizeBig,   sunSizeBig,   0.92f, 0.43f }, // TR 2
		{  sunSizeBig,   sunSizeBig,   0.92f, 0.43f }, // BR 3
		             // Small sun square
		{ -sunSizeLow,  -sunSizeLow,   0.89f, 0.77f }, // TL 4
		{  sunSizeLow,  -sunSizeLow,   0.89f, 0.77f }, // BL 5
		{ -sunSizeLow,   sunSizeLow,   0.89f, 0.77f }, // TR 6
		{  sunSizeLow,   sunSizeLow,   0.89f, 0.77f }, // BR 7
		             // Large moon square
		{ -moonSizeBig,  moonSizeBig, -0.59f, 0.94f }, // TR 8
		{  moonSizeBig,  moonSizeBig, -0.59f, 0.94f }, // BR 9
		{ -moonSizeBig, -moonSizeBig, -0.59f, 0.94f }, // TL 10
		{  moonSizeBig, -moonSizeBig, -0.59f, 0.94f }, // BL 11
		             // Small moon square
		{ -moonSizeLow,  moonSizeLow, -0.78f, 1.00f }, // TR 12
		{  moonSizeLow,  moonSizeLow, -0.78f, 1.00f }, // BR 13
		{ -moonSizeLow, -moonSizeLow, -0.78f, 1.00f }, // TL 14
		{  moonSizeLow, -moonSizeLow, -0.78f, 1.00f }, // BL 15
	}; // (Use 'Rcol' sign to determine Y position instead of using another attribute)

	// An EBO (vertice indexes) needs to be used with GL_TRIANGLES as the sun and
	// moon are seperate objects and using triangle strips would join them together.
	// Using plain glDrawArrays means some vertices will need to be redefined.

	const std::uint8_t planetsIndices[] = {
		/* Large sun indices  */ 0u,  1u,  3u,  3u,  2u,  0u,
		/* Small sun indices  */ 4u,  5u,  7u,  7u,  6u,  4u,
		/* Large moon indices */ 8u,  9u,  11u, 11u, 10u, 8u,
		/* Small moon indices */ 12u, 13u, 15u, 15u, 14u, 12u
	};

	m_planetsEBO = OGL::CreateBuffer(GL_ELEMENT_ARRAY_BUFFER);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(planetsIndices)), planetsIndices, GLbitfield{});

	// Buffer sun and moon data to array buffer
	glBufferStorage(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(planetsData)), planetsData, GLbitfield{});
}

Skybox::~Skybox() noexcept
{
	// Delete all created buffers (VBOs, EBOs)
	const GLuint deleteBuffers[] = {
		m_cloudsInstVBO,
		m_cloudsShapeVBO,
		m_cloudsShapeEBO,
		m_skyboxVBO,
		m_skyboxEBO,
		m_starsVBO,
		m_planetsVBO,
		m_planetsEBO
	};

	glDeleteBuffers(static_cast<GLsizei>(Math::size(deleteBuffers)), deleteBuffers);

	// Delete all created VAOs
	const GLuint deleteVAOs[] = {
		m_cloudsVAO,
		m_skyboxVAO,
		m_starsVAO,
		m_planetsVAO
	};

	glDeleteVertexArrays(static_cast<GLsizei>(Math::size(deleteVAOs)), deleteVAOs);
}
