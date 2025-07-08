#version 430 core

uniform vec4 texturePositions[4] = { 
    // vec4( X1, X2 - X1, Y1, Y2 - Y1 )

	// Grey background (random dark pixel)
    vec4( 0.50, 0.0, 0.2, 0.0 ),
	// Unequipped slot
    vec4( 0.0, 1.0, 0.0, 0.42857142857142857142857142857143),
	// Equipped slot
    vec4( 0.0, 1.0, 0.42857142857142857142857142857143, 0.42857142857142857142857142857143),
	// Center crosshair
    vec4( 0.0, 0.1666666666666666666666666666666, 0.92857142857142857142857142857143, 0.07142857142857142857142857142857)
};

layout (std140, binding = 4) uniform GameSizes {
	float blockTextureSize;
	float inventoryTextureSize;
};

layout (location = 0) in vec4 d1; // X, Y, W, H
layout (location = 1) in uint d2; // Texture + type
layout (location = 2) in vec3 baseGUI;

out vec2 TexCoord;
out flat int which;

void main()
{
	const uint texID = d2 >> 1;
	
	if ((d2 & 1) == 1) {
		which = 1;
		TexCoord = vec2((baseGUI.x + float(texID)) * blockTextureSize, baseGUI.z);
	} else {
        which = 0;
        const vec4 texPositions = texturePositions[texID];
		TexCoord = vec2(
			(baseGUI.x * texPositions.y) + texPositions.x, 
			(baseGUI.y * texPositions.w) + texPositions.z
		);
	}

    gl_Position = vec4(
		d1.x + (baseGUI.x * d1.z),
		d1.y + (baseGUI.y * d1.w), 
		0.0, 1.0
	);
}
