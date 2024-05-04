#include <Scene.h>

void Astra::Scene::draw()
{
	// pass
}

void Astra::Scene::addNode(Node3D* n)
{
	_nodes.push_back(n);
}

void Astra::Scene::removeNode(const Node3D& n)
{
	auto eraser = _nodes.begin();
	bool found = false;
	for (auto it = _nodes.begin(); it != _nodes.end() && !found; ++it) {
		if (*(*it) == n) {
			eraser = it;
			found = true;
		}
	}
	if (found)	_nodes.erase(eraser);
}
