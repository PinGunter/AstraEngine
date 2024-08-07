#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_debug_printf : enable

#include "raycommon.glsl"
#include "wavefront.glsl"

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 1) rayPayloadEXT bool isShadowed;

layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices {ivec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar) buffer Materials {WaveFrontMaterial m[]; }; // Array of all materials on an object
layout(buffer_reference, scalar) buffer MatIndices {int i[]; }; // Material ID for each triangle
layout(set = 0, binding = eTlas) uniform accelerationStructureEXT topLevelAS;
layout(set = 1, binding = eObjDescs, scalar) buffer ObjDesc_ { ObjDesc i[]; } objDesc;
layout(set = 1, binding = eTextures) uniform sampler2D textureSamplers[];
layout(set = 1, binding = eLights) uniform _LightsUniform { LightsUniform lightUni; };

layout(push_constant) uniform _PushConstantRay { PushConstantRay pcRay; };

void main()
{
	// object data
	ObjDesc objResource = objDesc.i[gl_InstanceCustomIndexEXT];
	MatIndices matIndices = MatIndices(objResource.materialIndexAddress);
	Materials materials = Materials (objResource.materialAddress);
	Indices indices = Indices(objResource.indexAddress);
	Vertices vertices = Vertices(objResource.vertexAddress);

	// indices of the triangle
	ivec3 ind = indices.i[gl_PrimitiveID];

	// vertex of the triangle
	Vertex v0 = vertices.v[ind.x];
	Vertex v1 = vertices.v[ind.y];
	Vertex v2 = vertices.v[ind.z];
	
	const vec3 barycentrics = vec3(1.0 - attribs.x  -attribs.y, attribs.x, attribs.y);
	
	// Computing the coordinates of the hit position
	const vec3 pos      = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
	const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));  // Transforming the position to world space

	// Computing normal
	const vec3 nrm = v0.nrm * barycentrics.x + v1.nrm * barycentrics.y + v2.nrm * barycentrics.z;
	vec3 worldNrm = normalize(vec3(nrm * gl_WorldToObjectEXT));

	// Material of the object
	int matIdx = matIndices.i[gl_PrimitiveID];
	WaveFrontMaterial mat = materials.m[matIdx];

    float tMin   = 0.001;
    float tMax   = 100000.0;

// ========= LIGHNING ========================
    vec3 diffuseColor = vec3(0);
    vec3 specularColor = vec3(0);
    float attenuation = 1;


    for (int i=0; i < pcRay.nLights; i++){
        // Vector toward the light
        vec3  L;
        float lightIntensity = lightUni.lights[i].intensity;
        float lightDistance  = 100000.0;
        // Point light
        if(lightUni.lights[i].type == 0)
        {
            vec3 lDir      = lightUni.lights[i].position - worldPos;
            lightDistance  = length(lDir);
            lightIntensity = lightUni.lights[i].intensity / (lightDistance * lightDistance);
            L              = normalize(lDir);
        }
        else  // Directional light
        {
            L = normalize(lightUni.lights[i].position);
        }

        // Diffuse
       diffuseColor += computeDiffuse(mat, L, lightUni.lights[i].color , worldNrm) * lightIntensity;
        if(mat.textureId >= 0)
        {
            uint txtId    = mat.textureId + objDesc.i[gl_InstanceCustomIndexEXT].txtOffset;
            vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;
            diffuseColor *= texture(textureSamplers[nonuniformEXT(txtId)], texCoord).xyz;
        }

        specularColor += computeSpecular(mat, gl_WorldRayDirectionEXT, L,lightUni.lights[i].color, worldNrm) * lightIntensity;

        // Tracing shadow ray only if the light is visible from the surface
        if(pcRay.shadows && dot(worldNrm, L) > 0)
        {

            vec3  origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
            vec3  rayDir = L;
            uint  flags  = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
            isShadowed   = true;
            traceRayEXT(topLevelAS,  // acceleration structure
                        flags,       // rayFlags
                        0xFF,        // cullMask
                        0,           // sbtRecordOffset
                        0,           // sbtRecordStride
                        1,           // missIndex
                        origin,      // ray origin
                        tMin,        // ray min range
                        rayDir,      // ray direction
                        tMax,        // ray max range
                        1            // payload (location = 1)
            );

            if(isShadowed)
            {
                attenuation *= .3f;
                specularColor = vec3(0);
            }
        }
  }


	if ((mat.illum == 5 || mat.illum == 6 || mat.illum == 9 ) && prd.depth < pcRay.maxDepth){
			vec3 origin = worldPos;

			float roi = 1.0 / 1.31f;
			float eta;
			if (dot(gl_WorldRayDirectionEXT, worldNrm) > 0.0f){
				worldNrm *= -1;
				eta = 1.0f / roi;
			} else{
				eta = roi;
			}

			vec3 rayDir = refract(gl_WorldRayDirectionEXT, worldNrm, eta);
			prd.depth++;

			traceRayEXT(topLevelAS,
						gl_RayFlagsNoneEXT,
						0xFF,
						0,0,0,
						origin,
						tMin,
						rayDir,
						tMax,
						0
			);
			prd.depth--;
	}
	
    prd.hitValue = vec3(attenuation * (diffuseColor + specularColor)) * prd.attenuation;

}

