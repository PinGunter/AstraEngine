/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <glm/glm.hpp>
#include "tiny_obj_loader.h"
#include <array>
#include <iostream>
#include <stdint.h>
#include <unordered_map>
#include <vector>
#include <host_device.h>

struct shapeObj
{
	uint32_t offset;
	uint32_t nbIndex;
	uint32_t matIndex;
};

class ObjLoader
{
public:
	void loadModel(const std::string& filename);

	std::vector<Vertex>   m_vertices;
	std::vector<uint32_t>    m_indices;
	std::vector<WaveFrontMaterial> m_materials;
	std::vector<std::string> m_textures;
	std::vector<int32_t>     m_matIndx;
};
