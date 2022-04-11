#pragma once
#include "ColliderComponent.h"
#include <vector>
class PhysicsSystem
{
public:
	static void Create();
	static void Destroy();
	static PhysicsSystem* Instance();

	void Update(float deltaTime);
	void RemoveCollider(ColliderComponent*);
	void AddCollider(ColliderComponent*);
private:
	PhysicsSystem() {};
	~PhysicsSystem() {};
	static PhysicsSystem* instance_;

	std::vector<ColliderComponent*> colliders_;
	std::vector<ColliderComponent*> pendColliders_;

	void ResolveConflict();
};

