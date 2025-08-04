#include "Sky.hpp"

Skybox::Skybox() noexcept
{
	CreateClouds();
	CreateStars();
	CreateSkybox();
	CreatePlanets();
}

void Skybox::RenderSkyboxElements() noexcept
{
	// Sky (orange section at sunrise/set)
	glDepthRange(1.0, 1.0); // Force the sky to render at the back

	game.shaders.programs.sky.Use();
	glBindVertexArray(m_skyboxVAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, nullptr);

	glDepthRange(0.0, 1.0); // Normal depth range

	// Stars
	game.shaders.programs.stars.Use();
	glBindVertexArray(m_starsVAO);
	glDrawArrays(GL_POINTS, 0, numStars);

	// Planets (sun and moon)
	game.shaders.programs.planets.Use();
	glBindVertexArray(m_planetsVAO);
	glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_BYTE, nullptr);
	
	// Clouds
	game.shaders.programs.clouds.Use();
	glBindVertexArray(m_cloudsVAO);
	glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, nullptr, numClouds);
}

void Skybox::CreateClouds() noexcept
{
	m_cloudsVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);
	glEnableVertexAttribArray(1u);
	glVertexAttribDivisor(1u, 1u); // Instanced rendering

	// Cloud shape data
	const float cloudVertices[24] =  {
		2.5f, 1.0f, 2.5f,  // Front-top-right     0
		0.0f, 1.0f, 2.5f,  // Front-top-left      1
		2.5f, 0.0f, 2.5f,  // Front-bottom-right  2
		0.0f, 0.0f, 2.5f,  // Front-bottom-left   3
		2.5f, 1.0f, 0.0f,  // Back-top-right      4
		0.0f, 1.0f, 0.0f,  // Back-top-left       5
		2.5f, 0.0f, 0.0f,  // Back-bottom-right   6
		0.0f, 0.0f, 0.0f,  // Back-bottom-left    7
	};

	// Buffer for cloud vertices
	m_cloudsVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(cloudVertices)), cloudVertices, GLbitfield{});
	
	// Buffer for indexes into cloud vertices
	const GLubyte cloudIndices[36] = {
		0u, 1u, 2u,  1u, 3u, 2u, 
		3u, 5u, 7u,  5u, 3u, 1u, 
		4u, 6u, 7u,  5u, 4u, 7u, 
		5u, 1u, 4u,  0u, 4u, 1u, 
		4u, 0u, 6u,  6u, 0u, 2u, 
		2u, 3u, 6u,  3u, 7u, 6u
	};
	m_cloudsEBO = OGL::CreateBuffer(GL_ELEMENT_ARRAY_BUFFER);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cloudIndices), cloudIndices, GL_STATIC_DRAW);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(cloudIndices)), cloudIndices, GLbitfield{});
	
	// Setup instanced cloud buffer
	m_cloudsInstVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(1u, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	
	// Random float generator in range [0.0f, 1.0f) for cloud positions and sizes
	std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> dist;

	// Cloud generation
	glm::vec4 *cloudData = new glm::vec4[numClouds]; // Cloud data
	const float XZmultiplier = 2000.0f, XZhalf = XZmultiplier * 0.5f, sizeMult = 30.0f;
	
	for (int i = 0; i < numClouds; ++i) {
		// Add new cloud properties to the cloud data
		cloudData[i] = {
			(dist(gen) * XZmultiplier) - XZhalf,       // X position
			198.7f + (static_cast<float>(i) * 0.07f),  // Y position
			(dist(gen) * XZmultiplier) - XZhalf,       // Z position
			glm::max(dist(gen), 0.02f) * sizeMult      // Size
		};
	}

	// Buffer the float data into the GPU (in instanced buffer)
	glBufferStorage(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(glm::vec4) * static_cast<std::size_t>(numClouds)), cloudData, GLbitfield{});

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
	const GLubyte skyboxIndices[] = {
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
	/* 
	    Using the Fibonnaci sphere algorithm gives evenly distributed positions
	    on a sphere rather than a large proportion being at the top and bottom. 
	    Random samples are taken so the stars do not appear in a pattern.
	    
	    phi = pi * (sqrt(5) - 1) = 3.883222077...
	    i = [0, max)
	    
	    theta = phi * i

	    y = 1 - (i / max) * 2
	    radius = sqrt(1 - y * y)
	    
	    x = cos(theta) * radius
	    z = sin(theta) * radius

	    This algorithm is executed in a compute shader, where the GPU will
	    create large numbers of stars in parallel.
	*/
	
	// Create and use the compute shader
	ShadersObject::Program compShader("Stars.comp");
	compShader.Use();

	// Create SSBO for star data results
	const GLuint ssbo = OGL::CreateBuffer(GL_SHADER_STORAGE_BUFFER);
	const GLsizeiptr ssboBytesSize = static_cast<GLsizeiptr>(sizeof(glm::vec4) * static_cast<std::size_t>(numStars));
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, ssboBytesSize, nullptr, GLbitfield{});
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1u, ssbo);

	std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> dist(0.4f, 0.6f);

	// Set uniform variables required in compute shader
	glUniform1ui(compShader.GetLocation("numStarsUI"), static_cast<GLuint>(numStars)); // (for invocation bounds checking)
	glUniform1f(compShader.GetLocation("initRandom"), dist(gen)); // (to avoid getting the same star patterns every launch)
	
	// Execute compute shader
	glDispatchCompute(static_cast<GLuint>(Math::roundUpX(numStars / 32, 32)), 1u, 1u);

	// Whilst it is executing...

	// Create stars VAO
	m_starsVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);

	// Create buffer for star data and allocate enough memory for it
	m_starsVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, 0, nullptr); // Vec3 on attribute 0
	
	// Create vec4 array for star data (not a vec3 array for reasons explained below,
	// using a float array to access each individual element)
	float *ssboData = new float[static_cast<std::size_t>(numStars) * static_cast<std::size_t>(4u)];
	
	// Wait for compute shader to finish all execution
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	
	// Copy final results from SSBO to stars VBO through vec4 array -
	// Vec3s have the same alignment as vec4, so just copy without
	// including the last (.w/4th) member.
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, GLintptr{}, ssboBytesSize, ssboData);

	// Move each adjacent 'vec4' to overlap the last member of current 'vec4' to essentially turn it into an array of vec3s.
	for (int i = 1; i < numStars; ++i) std::memmove(ssboData + (i * 3), ssboData + (i * 4), sizeof(glm::vec3));

	// Buffer new vec3 data into actual star shader data (only the vec3 section)
	glBufferStorage(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(glm::vec3) * static_cast<std::size_t>(numStars)), ssboData, GLbitfield{});

	// Delete created SSBO and float array, compute shader is destroyed on scope leave
	glDeleteBuffers(1, &ssbo);
	delete[] ssboData;
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

	// An EBO (indexes into above vertices) needs to be used with GL_TRIANGLES as the sun and
	// moon are seperate objects and using GL_TRIANGLE_STRIPS would join them together.
	// Using plain glDrawArrays means some vertices will need to be redefined - save memory with EBO.

	const GLubyte planetsIndices[] = {
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
		m_cloudsVBO,
		m_cloudsEBO,
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
