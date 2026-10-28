#pragma once
#include "Body.hpp"
#include "Rope.hpp"
#include <vector>
#include <string>

struct ContactConstraint {
    int idA = -1;
    int idB = -1;
    Vec2 point = {0.0f, 0.0f};
    Vec2 normal = {0.0f, 1.0f};
    Vec2 tangent = {1.0f, 0.0f};
    Vec2 rA = {0.0f, 0.0f};
    Vec2 rB = {0.0f, 0.0f};
    float penetration = 0.0f;
    float effMassN = 0.0f;
    float effMassT = 0.0f;
    float friction = 0.5f;
    float restitution = 0.0f;
    float restitutionVel = 0.0f;
    float jnAccum = 0.0f;
    float jtAccum = 0.0f;
};

struct WorldSnapshot {
    std::vector<Body> bodies;
    std::vector<RopeLink> ropes;
    std::vector<PulleyLink> pulleys;
    float simTime = 0.0f;
    bool valid = false;
};

class PhysicsWorld {
public:
    float G = 9.81f;
    int substeps = 24;
    float simTime = 0.0f;
    int defaultTrailMode = 1;
    float defaultTrailLifetime = 5.0f;

    int integratorType = 0;
    bool airResistance = false;
    int activePreset = 0;

    float lastStepTimeMs = 0.0f;
    int broadphasePairs = 0;
    int narrowphaseTests = 0;
    int activeContactsCount = 0;

    int trailMode = 0;
    float trailDuration = 2.5f;

    std::vector<Body> bodies;
    std::vector<RopeLink> ropes;
    std::vector<PulleyLink> pulleys;
    WorldSnapshot snapshot;

    float groundY = 0.5f;

    int createCircle(Vec2 pos, float radius, float mass, bool isStatic = false);
    int createBody(Vec2 pos, float w, float h, float mass, bool isStatic = false);
    int addRope(int idA, int idB, Vec2 offA, Vec2 offB, float length, float stiffness = 180.0f, bool isRigid = false, bool isSpring = false);
    int addFreeRope(int idA, Vec2 offA, Vec2 endWorldPos, float length, float stiffness = 180.0f, bool isRigid = false, bool isSpring = false);
    int addPulley(int idA, int idB, Vec2 offA, Vec2 offB, Vec2 wheelPos, float radius, float totalLen);

    Vec2 findSafeSpawnPos(Vec2 desiredPos, float w, float h) const;
    bool tryAttachBodyToFreeRopes(int bodyId);
    void syncRopeLengthToAttachedBody(int ropeId);
    void resolveEditorOverlap(int draggedId);

    void removeBody(int id);
    void removeRope(int id);
    void removePulley(int id);
    void clearAllTrails();
    void clearScene();
    void setupDefaultScene();
    void loadLabPreset(int presetId);

    void saveSnapshot();
    void restoreSnapshot();

    void step(float dt, int draggedBodyId, const Vec2& targetBodyPos, int draggedFreeRopeId = -1);
    void updateStaticTelemetry();

    Vec2 getSnapOffset(int bodyId, const Vec2& worldClick) const;
    Vec2 getRopeEndWorldPos(const RopeLink& r) const;

    bool computeCenterOfMass(Vec2& outPos, Vec2& outVel, float& outTotalMass) const;
    bool saveSceneToFile(const std::string& filename) const;
    bool loadSceneFromFile(const std::string& filename);

private:
    std::vector<ContactConstraint> contacts;

    void buildContacts();
    void solveVelocityConstraints(float subDt, std::vector<float>& ropeImpulseAccum, std::vector<float>& pulleyImpulseAccum);
    void solvePositionConstraints(int pinnedId = -1);
    void enforceBoundaries();
};