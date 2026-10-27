#include "PhysicsWorld.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>

struct OBBManifold {
    bool hit = false;
    Vec2 normal = {0.0f, 1.0f};
    float depth = 0.0f;
    Vec2 points[2];
    float depths[2] = {0.0f, 0.0f};
    int pointCount = 0;
};

static void ProjectOBB(const Body& b, const Vec2& axis, float& minP, float& maxP) {
    auto verts = b.getVertices();
    minP = maxP = Vec2::dot(verts[0], axis);
    for (int i = 1; i < 4; ++i) {
        float p = Vec2::dot(verts[i], axis);
        if (p < minP) minP = p;
        if (p > maxP) maxP = p;
    }
}

static int ClipSegmentToLine(const Vec2 vIn[2], Vec2 vOut[2], const Vec2& normal, float offset) {
    int numOut = 0;
    float d0 = Vec2::dot(normal, vIn[0]) - offset;
    float d1 = Vec2::dot(normal, vIn[1]) - offset;

    if (d0 <= 0.0f) vOut[numOut++] = vIn[0];
    if (d1 <= 0.0f) vOut[numOut++] = vIn[1];

    if (d0 * d1 < 0.0f) {
        float t = d0 / (d0 - d1);
        vOut[numOut++] = vIn[0] + (vIn[1] - vIn[0]) * t;
    }
    return numOut;
}

static void GetSupportEdge(const Body& b, const Vec2& dir, Vec2 outEdge[2]) {
    auto verts = b.getVertices();
    int bestIndex = 0;
    float maxDot = Vec2::dot(verts[0], dir);
    for (int i = 1; i < 4; ++i) {
        float d = Vec2::dot(verts[i], dir);
        if (d > maxDot) {
            maxDot = d;
            bestIndex = i;
        }
    }

    Vec2 v = verts[bestIndex];
    Vec2 vPrev = verts[(bestIndex + 3) % 4];
    Vec2 vNext = verts[(bestIndex + 1) % 4];

    Vec2 ePrev = (v - vPrev).normalized();
    Vec2 eNext = (v - vNext).normalized();

    if (std::abs(Vec2::dot(ePrev, dir)) <= std::abs(Vec2::dot(eNext, dir))) {
        outEdge[0] = vPrev;
        outEdge[1] = v;
    } else {
        outEdge[0] = v;
        outEdge[1] = vNext;
    }
}

static OBBManifold CollideOBB(const Body& a, const Body& b, float margin) {
    OBBManifold m;
    Vec2 axes[4] = {a.axisX(), a.axisY(), b.axisX(), b.axisY()};

    float minOverlap = 1e9f;
    Vec2 bestAxis = {0.0f, 1.0f};
    int bestAxisIdx = -1;

    Vec2 dCenter = b.pos - a.pos;

    for (int i = 0; i < 4; ++i) {
        Vec2 axis = axes[i];
        float minA, maxA, minB, maxB;
        ProjectOBB(a, axis, minA, maxA);
        ProjectOBB(b, axis, minB, maxB);

        float overlap = std::min(maxA, maxB) - std::max(minA, minB);
        if (overlap < -margin) return m;

        if (bestAxisIdx == -1 || overlap < minOverlap - 1e-5f) {
            minOverlap = overlap;
            bestAxis = (Vec2::dot(dCenter, axis) >= 0.0f) ? axis : (axis * -1.0f);
            bestAxisIdx = i;
        }
    }

    if (std::abs(bestAxis.x) < 0.008f) {
        bestAxis = {0.0f, (bestAxis.y >= 0.0f) ? 1.0f : -1.0f};
    } else if (std::abs(bestAxis.y) < 0.008f) {
        bestAxis = {(bestAxis.x >= 0.0f) ? 1.0f : -1.0f, 0.0f};
    }

    m.hit = true;
    m.normal = bestAxis;
    m.depth = std::max(0.0f, minOverlap);

    const Body* refBody = (bestAxisIdx < 2) ? &a : &b;
    const Body* incBody = (bestAxisIdx < 2) ? &b : &a;
    Vec2 refNormal = (bestAxisIdx < 2) ? bestAxis : (bestAxis * -1.0f);

    Vec2 refEdge[2], incEdge[2];
    GetSupportEdge(*refBody, refNormal, refEdge);
    GetSupportEdge(*incBody, refNormal * -1.0f, incEdge);

    Vec2 sideDir = (refEdge[1] - refEdge[0]).normalized();
    float o1 = Vec2::dot(sideDir, refEdge[0]);
    float o2 = Vec2::dot(sideDir, refEdge[1]);

    Vec2 clip1[2], clip2[2];
    int np = ClipSegmentToLine(incEdge, clip1, sideDir * -1.0f, -o1);
    if (np < 2) return m;

    np = ClipSegmentToLine(clip1, clip2, sideDir, o2);
    if (np < 1) return m;

    float refOffset = Vec2::dot(refNormal, refEdge[0]);
    for (int i = 0; i < np; ++i) {
        float sep = Vec2::dot(refNormal, clip2[i]) - refOffset;
        if (sep <= margin) {
            m.points[m.pointCount] = clip2[i];
            m.depths[m.pointCount] = std::max(0.0f, -sep);
            m.pointCount++;
            if (m.pointCount == 2) break;
        }
    }

    if (m.pointCount == 0) {
        m.points[0] = (incEdge[0] + incEdge[1]) * 0.5f;
        m.depths[0] = m.depth;
        m.pointCount = 1;
    }

    return m;
}

static OBBManifold CollideCircleCircle(const Body& a, const Body& b, float margin) {
    OBBManifold m;
    Vec2 d = b.pos - a.pos;
    float distSq = d.lengthSq();
    float rSum = a.radius + b.radius;

    if (distSq > (rSum + margin) * (rSum + margin)) return m;

    float dist = std::sqrt(distSq);
    m.hit = true;
    m.pointCount = 1;

    if (dist > 1e-5f) {
        m.normal = d / dist;
        m.depth = std::max(0.0f, rSum - dist);
        m.depths[0] = m.depth;
        m.points[0] = a.pos + m.normal * (a.radius - m.depth * 0.5f);
    } else {
        m.normal = {0.0f, 1.0f};
        m.depth = rSum;
        m.depths[0] = m.depth;
        m.points[0] = a.pos;
    }
    return m;
}

static OBBManifold CollideCircleOBB(const Body& circle, const Body& obb, float margin) {
    OBBManifold m;
    Vec2 localC = obb.worldToLocal(circle.pos);
    float hx = obb.width * 0.5f;
    float hy = obb.height * 0.5f;

    Vec2 closest = {
        std::clamp(localC.x, -hx, hx),
        std::clamp(localC.y, -hy, hy)
    };

    Vec2 dLocal = closest - localC;
    float distSq = dLocal.lengthSq();

    if (distSq > 1e-8f) {
        if (distSq > (circle.radius + margin) * (circle.radius + margin)) return m;
        float dist = std::sqrt(distSq);
        Vec2 localNormal = dLocal / dist;

        m.hit = true;
        m.normal = localNormal.rotated(obb.angle);
        m.depth = std::max(0.0f, circle.radius - dist);
        m.depths[0] = m.depth;
        m.pointCount = 1;
        m.points[0] = obb.localToWorld(closest);
    } else {
        float dx = hx - std::abs(localC.x);
        float dy = hy - std::abs(localC.y);
        Vec2 localNormal;

        if (dx < dy) {
            localNormal = {(localC.x >= 0.0f) ? -1.0f : 1.0f, 0.0f};
            closest.x = (localC.x >= 0.0f) ? hx : -hx;
            m.depth = circle.radius + dx;
        } else {
            localNormal = {0.0f, (localC.y >= 0.0f) ? -1.0f : 1.0f};
            closest.y = (localC.y >= 0.0f) ? hy : -hy;
            m.depth = circle.radius + dy;
        }

        m.hit = true;
        m.normal = localNormal.rotated(obb.angle);
        m.depths[0] = m.depth;
        m.pointCount = 1;
        m.points[0] = obb.localToWorld(closest);
    }

    return m;
}

static OBBManifold CollideBodies(const Body& a, const Body& b, float margin = 0.004f) {
    if (a.shape == ShapeType::CIRCLE && b.shape == ShapeType::CIRCLE) {
        return CollideCircleCircle(a, b, margin);
    } else if (a.shape == ShapeType::CIRCLE && b.shape == ShapeType::BOX) {
        return CollideCircleOBB(a, b, margin);
    } else if (a.shape == ShapeType::BOX && b.shape == ShapeType::CIRCLE) {
        OBBManifold m = CollideCircleOBB(b, a, margin);
        if (m.hit) m.normal = m.normal * -1.0f;
        return m;
    }
    return CollideOBB(a, b, margin);
}

int PhysicsWorld::createBody(Vec2 pos, float w, float h, float mass, bool isStatic) {
    int id = (int)bodies.size();
    bodies.emplace_back(id, pos, w, h, mass, isStatic);
    return id;
}

int PhysicsWorld::createCircle(Vec2 pos, float radius, float mass, bool isStatic) {
    Body b;
    b.id = (int)bodies.size();
    b.shape = ShapeType::CIRCLE;
    b.pos = pos;
    b.radius = radius;
    b.width = radius * 2.0f;
    b.height = radius * 2.0f;
    b.mass = mass;
    b.friction = 0.5f;
    b.restitution = 0.35f;
    b.isStatic = isStatic;
    b.name = "Шар";
    b.updateMassData();
    bodies.push_back(b);
    return b.id;
}

int PhysicsWorld::addRope(int idA, int idB, Vec2 offA, Vec2 offB, float length, float stiffness, bool isRigid, bool isSpring) {
    if (idA == idB || idA < 0) return -1;
    int id = (int)ropes.size();
    ropes.emplace_back(id, idA, idB, offA, offB, length, stiffness, isRigid, isSpring);
    return id;
}

int PhysicsWorld::addFreeRope(int idA, Vec2 offA, Vec2 endWorldPos, float length, float stiffness, bool isRigid, bool isSpring) {
    if (idA < 0 || idA >= (int)bodies.size()) return -1;
    int id = (int)ropes.size();
    RopeLink r(id, idA, -1, offA, {0.0f, 0.0f}, length, stiffness, isRigid, isSpring);

    Vec2 pA = bodies[idA].localToWorld(offA);
    Vec2 delta = endWorldPos - pA;
    Vec2 dir = (delta.length() > 1e-4f) ? delta.normalized() : Vec2{0.0f, -1.0f};
    r.freePos = pA + dir * length;
    r.currentLength = length;
    r.dir = dir * -1.0f;

    ropes.push_back(r);
    return id;
}

Vec2 PhysicsWorld::getRopeEndWorldPos(const RopeLink& r) const {
    if (r.idB >= 0 && r.idB < (int)bodies.size()) {
        return bodies[r.idB].localToWorld(r.offsetB);
    }
    return r.freePos;
}

Vec2 PhysicsWorld::findSafeSpawnPos(Vec2 desiredPos, float w, float h) const {
    Body temp(-1, desiredPos, w, h, 1.0f, false);
    if (temp.pos.y - h * 0.5f < groundY) {
        temp.pos.y = groundY + h * 0.5f;
    }

    for (int iter = 0; iter < 30; ++iter) {
        bool overlapped = false;
        for (const auto& b : bodies) {
            OBBManifold m = CollideBodies(b, temp, -0.0005f);
            if (m.hit && m.depth > 0.0001f) {
                if (!b.isStatic && std::abs(temp.pos.x - b.pos.x) < b.width * 0.35f && std::abs(b.angle) < 0.05f) {
                    temp.pos.x = b.pos.x;
                }
                temp.pos.y += m.depth;
                overlapped = true;
                break;
            }
        }
        if (!overlapped) break;
    }

    if (temp.pos.y - h * 0.5f < groundY) {
        temp.pos.y = groundY + h * 0.5f;
    }
    return temp.pos;
}

bool PhysicsWorld::tryAttachBodyToFreeRopes(int bodyId) {
    if (bodyId < 0 || bodyId >= (int)bodies.size()) return false;
    Body& b = bodies[bodyId];
    if (b.isStatic) return false;

    for (auto& r : ropes) {
        if (r.idB == -1 && r.idA != bodyId) {
            Vec2 localHook = b.worldToLocal(r.freePos);
            float margin = 0.35f;

            bool inside = false;
            if (b.shape == ShapeType::CIRCLE) {
                inside = (localHook.length() <= b.radius + margin);
            } else {
                inside = (std::abs(localHook.x) <= b.width * 0.5f + margin &&
                          std::abs(localHook.y) <= b.height * 0.5f + margin);
            }

            if (inside) {
                r.idB = bodyId;
                r.offsetB = getSnapOffset(bodyId, r.freePos);
                b.pos = r.freePos - r.offsetB.rotated(b.angle);
                b.vel = {0.0f, 0.0f};
                b.angVel = 0.0f;
                updateStaticTelemetry();
                return true;
            }
        }
    }
    return false;
}

void PhysicsWorld::syncRopeLengthToAttachedBody(int ropeId) {
    if (ropeId < 0 || ropeId >= (int)ropes.size()) return;
    RopeLink& r = ropes[ropeId];
    Vec2 pA = bodies[r.idA].localToWorld(r.offsetA);

    if (r.idB == -1) {
        Vec2 delta = r.freePos - pA;
        Vec2 n = (delta.length() > 1e-4f) ? delta.normalized() : Vec2{0.0f, -1.0f};
        r.freePos = pA + n * r.restLength;
        r.currentLength = r.restLength;
    } else if (!bodies[r.idB].isStatic) {
        Body& b = bodies[r.idB];
        Vec2 pB = b.localToWorld(r.offsetB);
        Vec2 delta = pB - pA;
        Vec2 n = (delta.length() > 1e-4f) ? delta.normalized() : Vec2{0.0f, -1.0f};
        float eqExt = r.isRigid ? 0.0f : ((n.y < -0.5f) ? (b.mass * G / r.stiffness) : 0.0f);
        Vec2 newPB = pA + n * (r.restLength + eqExt);
        b.pos = newPB - r.offsetB.rotated(b.angle);
        b.vel = {0.0f, 0.0f};
        b.angVel = 0.0f;

        float hY = (b.shape == ShapeType::CIRCLE) ? b.radius : (b.height * 0.5f);
        if (b.pos.y - hY < groundY) {
            b.pos.y = groundY + hY;
        }
    }
    updateStaticTelemetry();
}

void PhysicsWorld::resolveEditorOverlap(int draggedId) {
    solvePositionConstraints(draggedId);
    enforceBoundaries();
    updateStaticTelemetry();
}

void PhysicsWorld::removeBody(int id) {
    if (id < 0 || id >= (int)bodies.size()) return;

    ropes.erase(std::remove_if(ropes.begin(), ropes.end(), [&](const RopeLink& r) {
        return r.idA == id || r.idB == id;
    }), ropes.end());

    bodies.erase(bodies.begin() + id);

    for (int i = 0; i < (int)bodies.size(); ++i) bodies[i].id = i;
    for (int i = 0; i < (int)ropes.size(); ++i) {
        ropes[i].id = i;
        if (ropes[i].idA > id) ropes[i].idA--;
        if (ropes[i].idB > id) ropes[i].idB--;
    }
    updateStaticTelemetry();
}

void PhysicsWorld::removeRope(int id) {
    if (id < 0 || id >= (int)ropes.size()) return;
    ropes.erase(ropes.begin() + id);
    for (int i = 0; i < (int)ropes.size(); ++i) ropes[i].id = i;
    updateStaticTelemetry();
}

void PhysicsWorld::clearAllTrails() {
    for (auto& b : bodies) {
        b.clearTrail();
    }
}

void PhysicsWorld::clearScene() {
    bodies.clear();
    ropes.clear();
    contacts.clear();
    simTime = 0.0f;
}

void PhysicsWorld::setupDefaultScene() {
    clearScene();
    int anchorId = createBody({-1.0f, 7.3f}, 3.5f, 0.24f, 1.0f, true);
    float eqStretch = (5.0f * G) / 180.0f;
    float startY = 7.3f - 2.4f - eqStretch - 0.65f;
    int hangingId = createBody({-1.0f, startY}, 1.3f, 1.3f, 5.0f, false);
    createBody({-4.2f, 1.0f}, 1.0f, 1.0f, 2.0f, false);

    addRope(anchorId, hangingId, {0.0f, 0.0f}, {0.0f, 0.65f}, 2.4f, 180.0f, false, false);
    updateStaticTelemetry();
    saveSnapshot();
}

void PhysicsWorld::loadLabPreset(int presetId) {
    clearScene();
    const float DEG2RAD_VAL = 0.0174532925f;

    switch (presetId) {
        case 1: {
            float tilt = -20.0f * DEG2RAD_VAL;

            int rampTop = createBody({-1.2f, 5.8f}, 7.2f, 0.26f, 1.0f, true);
            bodies[rampTop].angle = tilt;
            bodies[rampTop].friction = 0.20f;

            Vec2 topStart = bodies[rampTop].localToWorld({-2.8f, 0.13f + 0.45f});
            int ballId = createCircle(topStart, 0.45f, 2.0f, false);
            bodies[ballId].friction = 0.20f;
            bodies[ballId].restitution = 0.05f;

            int rampBot = createBody({-1.2f, 2.6f}, 7.2f, 0.26f, 1.0f, true);
            bodies[rampBot].angle = tilt;
            bodies[rampBot].friction = 0.20f;

            Vec2 botStart = bodies[rampBot].localToWorld({-2.8f, 0.13f + 0.45f});
            int boxId = createBody(botStart, 0.9f, 0.9f, 2.0f, false);
            bodies[boxId].angle = tilt;
            bodies[boxId].friction = 0.20f;

            int stopper = createBody({4.9f, 1.9f}, 2.8f, 0.26f, 1.0f, true);
            bodies[stopper].angle = 70.0f * DEG2RAD_VAL;
            break;
        }
        case 2: {
            int beam = createBody({0.0f, 7.6f}, 8.5f, 0.26f, 1.0f, true);

            int b1 = createBody({-3.0f, 3.6f}, 1.1f, 1.1f, 3.0f, false);
            addRope(beam, b1, {-3.0f, -0.13f}, {0.0f, 0.55f}, 2.4f, 90.0f, false, true);

            int b2 = createBody({0.0f, 3.6f}, 1.1f, 1.1f, 3.0f, false);
            addRope(beam, b2, {0.0f, -0.13f}, {0.0f, 0.55f}, 2.4f, 320.0f, false, true);

            int ball = createCircle({4.5f, 5.6f}, 0.48f, 2.5f, false);
            addRope(beam, ball, {2.8f, -0.13f}, {0.0f, 0.0f}, 2.6f, 200.0f, true, false);
            break;
        }
        case 3: {
            float tilt = -18.0f * DEG2RAD_VAL;
            int ramp = createBody({-4.6f, 1.04f}, 4.2f, 0.26f, 1.0f, true);
            bodies[ramp].angle = tilt;
            bodies[ramp].friction = 0.0f;

            Vec2 launchPos = bodies[ramp].localToWorld({-1.4f, 0.13f + 0.5f});
            int striker = createCircle(launchPos, 0.5f, 2.0f, false);
            bodies[striker].restitution = 0.99f;
            bodies[striker].friction = 0.0f;
            bodies[striker].canRotate = false;

            for (int i = 0; i < 4; ++i) {
                int id = createCircle({-1.2f + i * 1.015f, groundY + 0.5f}, 0.5f, 2.0f, false);
                bodies[id].restitution = 0.99f;
                bodies[id].friction = 0.0f;
                bodies[id].canRotate = false;
            }
            break;
        }
        case 4: {
            int leftBase = createBody({-3.2f, groundY + 0.5f}, 1.4f, 1.0f, 2.0f, false);
            bodies[leftBase].friction = 0.5f;
            bodies[leftBase].appliedForceX = 18.0f;

            int topWeight = createBody({-3.2f, groundY + 1.5f}, 1.0f, 1.0f, 3.0f, false);
            bodies[topWeight].friction = 0.5f;

            int rightSingle = createBody({1.2f, groundY + 0.5f}, 1.4f, 1.0f, 2.0f, false);
            bodies[rightSingle].friction = 0.5f;
            bodies[rightSingle].appliedForceX = 18.0f;
            break;
        }
        default:
            setupDefaultScene();
            return;
    }

    resolveEditorOverlap(-1);
    updateStaticTelemetry();
    saveSnapshot();
}

void PhysicsWorld::saveSnapshot() {
    snapshot.bodies = bodies;
    for (auto& b : snapshot.bodies) {
        b.clearTrail();
    }
    snapshot.ropes = ropes;
    snapshot.simTime = 0.0f;
    snapshot.valid = true;
}

void PhysicsWorld::restoreSnapshot() {
    if (!snapshot.valid) return;
    bodies = snapshot.bodies;
    ropes = snapshot.ropes;
    simTime = 0.0f;
    for (auto& b : bodies) {
        b.vel = {0.0f, 0.0f};
        b.angVel = 0.0f;
        b.clearTrail();
    }
    updateStaticTelemetry();
}

Vec2 PhysicsWorld::getSnapOffset(int bodyId, const Vec2& worldClick) const {
    if (bodyId < 0 || bodyId >= (int)bodies.size()) return {0.0f, 0.0f};
    const Body& b = bodies[bodyId];
    Vec2 local = b.worldToLocal(worldClick);

    if (b.shape == ShapeType::CIRCLE) {
        if (local.length() < b.radius * 0.35f) return {0.0f, 0.0f};
        return local.normalized() * b.radius;
    }

    if (b.isStatic) {
        float clampedX = std::clamp(local.x, -b.width * 0.48f, b.width * 0.48f);
        return {clampedX, 0.0f};
    }

    float hw = b.width * 0.5f;
    float hh = b.height * 0.5f;

    Vec2 candidates[9] = {
        {0.0f, 0.0f},
        {0.0f,  hh}, {-hw * 0.85f,  hh}, {hw * 0.85f,  hh},
        {0.0f, -hh}, {-hw * 0.85f, -hh}, {hw * 0.85f, -hh},
        {-hw, 0.0f}, { hw, 0.0f}
    };

    Vec2 best = candidates[0];
    float bestDist = (local - best).length();
    for (int i = 1; i < 9; ++i) {
        float d = (local - candidates[i]).length();
        if (d < bestDist) {
            bestDist = d;
            best = candidates[i];
        }
    }
    return best;
}

void PhysicsWorld::step(float dt, int draggedBodyId, const Vec2& targetBodyPos, int draggedFreeRopeId) {
    if (trailMode > 0 && simTime == 0.0f) {
        for (auto& b : bodies) {
            if (!b.isStatic && b.trail.empty()) {
                b.trail.push_back({b.pos, 0.0f, true});
                b.lastStrobeTime = 0.0f;
            }
        }
    }

    simTime += dt;
    int steps = std::clamp(substeps, 4, 100);
    float subDt = dt / (float)steps;

    std::vector<float> frameNormalImp(bodies.size(), 0.0f);
    std::vector<float> frameTopImp(bodies.size(), 0.0f);
    std::vector<float> frameFrictionImp(bodies.size(), 0.0f);
    std::vector<float> frameRopeTensionSum(ropes.size(), 0.0f);

    for (int s = 0; s < steps; ++s) {
        // 1. Упругие нити и пружины
        for (size_t i = 0; i < ropes.size(); ++i) {
            RopeLink& r = ropes[i];
            Body& a = bodies[r.idA];
            Vec2 rA = r.offsetA.rotated(a.angle);
            Vec2 pA = a.pos + rA;

            if (r.idB == -1) {
                if ((int)i == draggedFreeRopeId) {
                    Vec2 d = targetBodyPos - pA;
                    float len = std::clamp(d.length(), 0.3f, 12.0f);
                    r.restLength = len;
                    r.currentLength = len;
                    Vec2 n = (d.length() > 1e-4f) ? d.normalized() : Vec2{0.0f, -1.0f};
                    r.freePos = pA + n * len;
                    r.freeVel = {0.0f, 0.0f};
                    r.dir = n * -1.0f;
                } else {
                    r.freeVel *= std::max(0.0f, 1.0f - 1.5f * subDt);
                    r.freeVel.y -= G * subDt;
                    r.freePos += r.freeVel * subDt;

                    Vec2 delta = r.freePos - pA;
                    float dist = delta.length();
                    if (dist > 1e-5f) {
                        Vec2 n = delta / dist;
                        r.freePos = pA + n * r.restLength;
                        float vn = Vec2::dot(r.freeVel, n);
                        if (vn > 0.0f) r.freeVel -= n * vn;
                        r.dir = n * -1.0f;
                        r.currentLength = r.restLength;
                    }
                }
                r.tension = 0.0f;
                continue;
            }

            Body& b = bodies[r.idB];
            Vec2 rB = r.offsetB.rotated(b.angle);
            Vec2 pB = b.pos + rB;

            Vec2 delta = pB - pA;
            float dist = delta.length();
            if (dist <= 1e-6f) continue;

            r.dir = (pA - pB) / dist;
            r.currentLength = dist;

            if (!r.isRigid) {
                float ext = dist - r.restLength;
                if (r.isSpring || ext > 0.0f) {
                    Vec2 n = delta / dist;
                    Vec2 vA = a.vel + Vec2::cross(a.angVel, rA);
                    Vec2 vB = b.vel + Vec2::cross(b.angVel, rB);
                    float vn = Vec2::dot(vB - vA, n);

                    float springF = r.stiffness * ext;
                    float dampF = r.damping * vn;
                    float totalF = r.isSpring ? (springF + dampF) : std::max(0.0f, springF + dampF);

                    Vec2 impulse = n * (totalF * subDt);
                    if (!a.isStatic) {
                        a.vel += impulse * a.invMass;
                        a.angVel += Vec2::cross(rA, impulse) * a.invInertia;
                    }
                    if (!b.isStatic) {
                        b.vel -= impulse * b.invMass;
                        b.angVel -= Vec2::cross(rB, impulse) * b.invInertia;
                    }

                    frameRopeTensionSum[i] += std::abs(springF);
                }
            }
        }

        // 2. Сопротивление воздуха, внешняя сила F и гравитация g
        for (auto& b : bodies) {
            if (b.isStatic) continue;

            float speed = b.vel.length();
            float airDrag = 0.05f + 0.02f * speed;
            float rotDrag = (b.shape == ShapeType::CIRCLE) ? 0.05f : 1.5f;

            b.vel *= std::max(0.0f, 1.0f - airDrag * subDt);
            b.angVel *= std::max(0.0f, 1.0f - rotDrag * subDt);

            b.vel.x += (b.appliedForceX * b.invMass) * subDt;
            b.vel.y -= G * subDt;

            if (b.id == draggedBodyId) {
                Vec2 err = targetBodyPos - b.pos;
                Vec2 desiredVel = err * 24.0f;
                float maxMouseSpeed = 12.0f;
                if (desiredVel.length() > maxMouseSpeed) {
                    desiredVel = desiredVel.normalized() * maxMouseSpeed;
                }

                Vec2 reqAcc = (desiredVel - b.vel) / subDt;
                Vec2 mouseForce = reqAcc * b.mass;
                float maxForce = std::max(180.0f, b.mass * G * 4.0f);
                if (mouseForce.length() > maxForce) {
                    mouseForce = mouseForce.normalized() * maxForce;
                }
                b.vel += mouseForce * (b.invMass * subDt);
                b.angVel *= 0.90f;
            }
        }

        buildContacts();
        std::vector<float> ropeRigidImp(ropes.size(), 0.0f);
        solveVelocityConstraints(subDt, ropeRigidImp);

        for (size_t i = 0; i < ropes.size(); ++i) {
            if (ropes[i].isRigid && ropes[i].idB != -1) {
                frameRopeTensionSum[i] += std::abs(ropeRigidImp[i]) / subDt;
            }
        }

        for (auto& b : bodies) {
            if (!b.isStatic) {
                b.pos += b.vel * subDt;
                if (b.canRotate) {
                    b.angle += b.angVel * subDt;
                } else {
                    b.angle = 0.0f;
                    b.angVel = 0.0f;
                }
            }
        }

        solvePositionConstraints(draggedBodyId);
        enforceBoundaries();

        std::vector<float> subNetFriction(bodies.size(), 0.0f);
        for (const auto& c : contacts) {
            if (c.jnAccum > 1e-6f) {
                if (c.idA >= 0 && bodies[c.idA].shape == ShapeType::CIRCLE && !bodies[c.idA].isStatic) {
                    bodies[c.idA].angVel *= std::max(0.0f, 1.0f - 0.35f * subDt);
                }
                if (c.idB >= 0 && bodies[c.idB].shape == ShapeType::CIRCLE && !bodies[c.idB].isStatic) {
                    bodies[c.idB].angVel *= std::max(0.0f, 1.0f - 0.35f * subDt);
                }
            }

            if (c.idA == -1) {
                frameNormalImp[c.idB] += c.jnAccum;
                subNetFriction[c.idB] += c.jtAccum;
            } else if (std::abs(c.normal.y) > 0.45f) {
                if (c.normal.y > 0.0f) {
                    frameNormalImp[c.idB] += c.jnAccum;
                    frameTopImp[c.idA] += c.jnAccum;
                    subNetFriction[c.idB] += c.jtAccum;
                } else {
                    frameNormalImp[c.idA] += c.jnAccum;
                    frameTopImp[c.idB] += c.jnAccum;
                    subNetFriction[c.idA] -= c.jtAccum;
                }
            }
        }

        for (size_t i = 0; i < bodies.size(); ++i) {
            frameFrictionImp[i] += std::abs(subNetFriction[i]);
        }
    }

    for (size_t i = 0; i < bodies.size(); ++i) {
        bodies[i].normalForce = frameNormalImp[i] / dt;
        bodies[i].topLoadForce = frameTopImp[i] / dt;

        float fTr = frameFrictionImp[i] / dt;
        if (bodies[i].vel.lengthSq() < 0.0004f && std::abs(bodies[i].appliedForceX) < 0.05f && fTr < 0.5f) {
            fTr = 0.0f;
        }
        bodies[i].frictionForce = fTr;
    }

    for (size_t i = 0; i < ropes.size(); ++i) {
        ropes[i].tension = frameRopeTensionSum[i] / (float)steps;
    }

    // Обработка трассеров согласно выбранному режиму (0 = Выкл, 1 = По времени, 2 = Оставлять всё)
    if (trailMode > 0) {
        for (auto& b : bodies) {
            if (b.isStatic) continue;

            if (trailMode == 1) {
                while (!b.trail.empty() && (simTime - b.trail.front().time > trailDuration)) {
                    b.trail.erase(b.trail.begin());
                }
            }

            bool needStrobe = (simTime - b.lastStrobeTime >= 0.10f);
            bool movedEnough = b.trail.empty() || ((b.pos - b.trail.back().pos).lengthSq() >= 0.0004f);

            if (movedEnough) {
                if (needStrobe) b.lastStrobeTime = simTime;
                b.trail.push_back({b.pos, simTime, needStrobe});
                if (trailMode == 2 && b.trail.size() > 4000) {
                    b.trail.erase(b.trail.begin());
                }
            } else if (needStrobe && !b.trail.empty()) {
                b.lastStrobeTime = simTime;
            }
        }
    }

    if (draggedBodyId >= 0) {
        tryAttachBodyToFreeRopes(draggedBodyId);
    }
}

void PhysicsWorld::buildContacts() {
    contacts.clear();
    const float MARGIN = 0.003f;

    auto initContactMasses = [&](ContactConstraint& c) {
        Body* a = (c.idA >= 0) ? &bodies[c.idA] : nullptr;
        Body& b = bodies[c.idB];

        if (a) {
            c.rA = (a->shape == ShapeType::CIRCLE) ? (c.normal * a->radius) : (c.point - a->pos);
        } else {
            c.rA = {0.0f, 0.0f};
        }
        c.rB = (b.shape == ShapeType::CIRCLE) ? (c.normal * (-b.radius)) : (c.point - b.pos);

        float wA = a ? a->invMass : 0.0f;
        float iA = a ? a->invInertia : 0.0f;
        float wB = b.invMass;
        float iB = b.invInertia;

        float rnA = Vec2::cross(c.rA, c.normal);
        float rnB = Vec2::cross(c.rB, c.normal);
        float kN = wA + wB + iA * rnA * rnA + iB * rnB * rnB;
        c.effMassN = (kN > 1e-6f) ? (1.0f / kN) : 0.0f;

        float rtA = Vec2::cross(c.rA, c.tangent);
        float rtB = Vec2::cross(c.rB, c.tangent);
        float kT = wA + wB + iA * rtA * rtA + iB * rtB * rtB;
        c.effMassT = (kT > 1e-6f) ? (1.0f / kT) : 0.0f;

        Vec2 vA = a ? (a->vel + Vec2::cross(a->angVel, c.rA)) : Vec2{0.0f, 0.0f};
        Vec2 vB = b.vel + Vec2::cross(b.angVel, c.rB);
        float vn = Vec2::dot(vB - vA, c.normal);

        float bounceThreshold = (c.idA == -1) ? -2.2f : -0.35f;
        if (vn < bounceThreshold) {
            c.restitutionVel = -c.restitution * vn;
        } else {
            c.restitutionVel = 0.0f;
        }
    };

    for (const auto& b : bodies) {
        if (b.isStatic) continue;

        if (b.shape == ShapeType::CIRCLE) {
            if (b.pos.y - b.radius <= groundY + MARGIN) {
                ContactConstraint c;
                c.idA = -1;
                c.idB = b.id;
                c.point = {b.pos.x, groundY};
                c.normal = {0.0f, 1.0f};
                c.tangent = {1.0f, 0.0f};
                c.penetration = std::max(0.0f, groundY - (b.pos.y - b.radius));
                c.friction = b.friction;
                c.restitution = b.restitution;
                initContactMasses(c);
                contacts.push_back(c);
            }
        } else {
            auto verts = b.getVertices();
            for (int k = 0; k < 4; ++k) {
                if (verts[k].y <= groundY + MARGIN) {
                    ContactConstraint c;
                    c.idA = -1;
                    c.idB = b.id;
                    c.point = verts[k];
                    c.normal = {0.0f, 1.0f};
                    c.tangent = {1.0f, 0.0f};
                    c.penetration = std::max(0.0f, groundY - verts[k].y);
                    c.friction = b.friction;
                    c.restitution = b.restitution;
                    initContactMasses(c);
                    contacts.push_back(c);
                }
            }
        }
    }

    std::vector<size_t> order(bodies.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](size_t i, size_t j) {
        return bodies[i].pos.y < bodies[j].pos.y;
    });

    for (size_t ii = 0; ii < order.size(); ++ii) {
        for (size_t jj = ii + 1; jj < order.size(); ++jj) {
            const Body& a = bodies[order[ii]];
            const Body& b = bodies[order[jj]];
            if (a.isStatic && b.isStatic) continue;

            OBBManifold m = CollideBodies(a, b, MARGIN);
            if (!m.hit) continue;

            Vec2 tangent = {-m.normal.y, m.normal.x};
            float mu = (a.shape == ShapeType::CIRCLE || b.shape == ShapeType::CIRCLE)
                     ? std::sqrt(a.friction * b.friction)
                     : std::min(a.friction, b.friction);
            float rest = std::max(a.restitution, b.restitution);

            for (int k = 0; k < m.pointCount; ++k) {
                ContactConstraint c;
                c.idA = a.id;
                c.idB = b.id;
                c.point = m.points[k];
                c.normal = m.normal;
                c.tangent = tangent;
                c.penetration = m.depths[k];
                c.friction = mu;
                c.restitution = rest;
                initContactMasses(c);
                contacts.push_back(c);
            }
        }
    }
}

void PhysicsWorld::solveVelocityConstraints(float subDt, std::vector<float>& ropeImpulseAccum) {
    const int SOLVER_ITERS = 18;

    auto solveFriction = [&](ContactConstraint& c) {
        Body* a = (c.idA >= 0) ? &bodies[c.idA] : nullptr;
        Body& b = bodies[c.idB];
        float wA = a ? a->invMass : 0.0f;
        float iA = a ? a->invInertia : 0.0f;
        float wB = b.invMass;
        float iB = b.invInertia;

        Vec2 vA = a ? (a->vel + Vec2::cross(a->angVel, c.rA)) : Vec2{0.0f, 0.0f};
        Vec2 vB = b.vel + Vec2::cross(b.angVel, c.rB);
        float vt = Vec2::dot(vB - vA, c.tangent);

        float dJt = -vt * c.effMassT;
        float maxFriction = c.friction * c.jnAccum;
        float newJt = std::clamp(c.jtAccum + dJt, -maxFriction, maxFriction);
        dJt = newJt - c.jtAccum;
        c.jtAccum = newJt;

        Vec2 impT = c.tangent * dJt;
        if (a && !a->isStatic) {
            a->vel -= impT * wA;
            a->angVel -= Vec2::cross(c.rA, impT) * iA;
        }
        if (!b.isStatic) {
            b.vel += impT * wB;
            b.angVel += Vec2::cross(c.rB, impT) * iB;
        }
    };

    for (int iter = 0; iter < SOLVER_ITERS; ++iter) {
        for (size_t i = 0; i < ropes.size(); ++i) {
            RopeLink& r = ropes[i];
            if (r.idB == -1) continue;

            Body& a = bodies[r.idA];
            Body& b = bodies[r.idB];

            Vec2 rA = r.offsetA.rotated(a.angle);
            Vec2 rB = r.offsetB.rotated(b.angle);
            Vec2 pA = a.pos + rA;
            Vec2 pB = b.pos + rB;
            Vec2 delta = pB - pA;
            float dist = delta.length();
            if (dist <= 1e-6f) continue;

            Vec2 n = delta / dist;
            float rnA = Vec2::cross(rA, n);
            float rnB = Vec2::cross(rB, n);
            float totalW = a.invMass + b.invMass + a.invInertia * rnA * rnA + b.invInertia * rnB * rnB;
            if (totalW <= 1e-6f) continue;

            if (r.isRigid && r.isSpring) {
                Vec2 vA = a.vel + Vec2::cross(a.angVel, rA);
                Vec2 vB = b.vel + Vec2::cross(b.angVel, rB);
                float vn = Vec2::dot(vB - vA, n);
                float bias = ((dist - r.restLength) * 0.15f) / subDt;
                float dLambda = (vn + bias) / totalW;
                ropeImpulseAccum[i] += dLambda;

                Vec2 imp = n * dLambda;
                if (!a.isStatic) {
                    a.vel += imp * a.invMass;
                    a.angVel += Vec2::cross(rA, imp) * a.invInertia;
                }
                if (!b.isStatic) {
                    b.vel -= imp * b.invMass;
                    b.angVel -= Vec2::cross(rB, imp) * b.invInertia;
                }
                continue;
            }

            float limitLen = r.isRigid ? r.restLength : (r.restLength + r.maxStretch);
            if (dist < limitLen - 0.002f) continue;

            Vec2 vA = a.vel + Vec2::cross(a.angVel, rA);
            Vec2 vB = b.vel + Vec2::cross(b.angVel, rB);
            float vn = Vec2::dot(vB - vA, n);

            float stretchErr = std::max(0.0f, dist - limitLen);
            float bias = (stretchErr * 0.15f) / subDt;

            float dLambda = (vn + bias) / totalW;
            float newImp = std::max(0.0f, ropeImpulseAccum[i] + dLambda);
            dLambda = newImp - ropeImpulseAccum[i];
            ropeImpulseAccum[i] = newImp;

            Vec2 imp = n * dLambda;
            if (!a.isStatic) {
                a.vel += imp * a.invMass;
                a.angVel += Vec2::cross(rA, imp) * a.invInertia;
            }
            if (!b.isStatic) {
                b.vel -= imp * b.invMass;
                b.angVel -= Vec2::cross(rB, imp) * b.invInertia;
            }
        }

        size_t i = 0;
        while (i < contacts.size()) {
            if (i + 1 < contacts.size() &&
                contacts[i].idA == contacts[i + 1].idA &&
                contacts[i].idB == contacts[i + 1].idB &&
                Vec2::dot(contacts[i].normal, contacts[i + 1].normal) > 0.99f) {

                ContactConstraint& c1 = contacts[i];
                ContactConstraint& c2 = contacts[i + 1];
                Body* a = (c1.idA >= 0) ? &bodies[c1.idA] : nullptr;
                Body& b = bodies[c1.idB];

                float wA = a ? a->invMass : 0.0f;
                float iA = a ? a->invInertia : 0.0f;
                float wB = b.invMass;
                float iB = b.invInertia;
                Vec2 n = c1.normal;

                float rnA1 = Vec2::cross(c1.rA, n);
                float rnB1 = Vec2::cross(c1.rB, n);
                float rnA2 = Vec2::cross(c2.rA, n);
                float rnB2 = Vec2::cross(c2.rB, n);

                float k11 = wA + wB + iA * rnA1 * rnA1 + iB * rnB1 * rnB1;
                float k22 = wA + wB + iA * rnA2 * rnA2 + iB * rnB2 * rnB2;
                float k12 = wA + wB + iA * rnA1 * rnA2 + iB * rnB1 * rnB2;

                Vec2 vA1 = a ? (a->vel + Vec2::cross(a->angVel, c1.rA)) : Vec2{0.0f, 0.0f};
                Vec2 vB1 = b.vel + Vec2::cross(b.angVel, c1.rB);
                float vn1 = Vec2::dot(vB1 - vA1, n);

                Vec2 vA2 = a ? (a->vel + Vec2::cross(a->angVel, c2.rA)) : Vec2{0.0f, 0.0f};
                Vec2 vB2 = b.vel + Vec2::cross(b.angVel, c2.rB);
                float vn2 = Vec2::dot(vB2 - vA2, n);

                float rhs1 = -vn1 + c1.restitutionVel + k11 * c1.jnAccum + k12 * c2.jnAccum;
                float rhs2 = -vn2 + c2.restitutionVel + k12 * c1.jnAccum + k22 * c2.jnAccum;

                float x1 = 0.0f, x2 = 0.0f;
                float det = k11 * k22 - k12 * k12;
                bool solved = false;

                if (det > 1e-6f) {
                    x1 = (k22 * rhs1 - k12 * rhs2) / det;
                    x2 = (k11 * rhs2 - k12 * rhs1) / det;
                    if (x1 >= 0.0f && x2 >= 0.0f) solved = true;
                }
                if (!solved) {
                    x1 = (k11 > 1e-6f) ? (rhs1 / k11) : 0.0f;
                    x2 = 0.0f;
                    if (x1 >= 0.0f && (k12 * x1 - rhs2) >= -1e-5f) solved = true;
                }
                if (!solved) {
                    x1 = 0.0f;
                    x2 = (k22 > 1e-6f) ? (rhs2 / k22) : 0.0f;
                    if (x2 >= 0.0f && (k12 * x2 - rhs1) >= -1e-5f) solved = true;
                }
                if (!solved) {
                    x1 = 0.0f;
                    x2 = 0.0f;
                }

                float dJn1 = x1 - c1.jnAccum;
                float dJn2 = x2 - c2.jnAccum;
                c1.jnAccum = x1;
                c2.jnAccum = x2;

                Vec2 imp1 = n * dJn1;
                Vec2 imp2 = n * dJn2;
                if (a && !a->isStatic) {
                    a->vel -= (imp1 + imp2) * wA;
                    a->angVel -= (Vec2::cross(c1.rA, imp1) + Vec2::cross(c2.rA, imp2)) * iA;
                }
                if (!b.isStatic) {
                    b.vel += (imp1 + imp2) * wB;
                    b.angVel += (Vec2::cross(c1.rB, imp1) + Vec2::cross(c2.rB, imp2)) * iB;
                }

                solveFriction(c1);
                solveFriction(c2);
                i += 2;
                continue;
            }

            ContactConstraint& c = contacts[i];
            Body* a = (c.idA >= 0) ? &bodies[c.idA] : nullptr;
            Body& b = bodies[c.idB];

            float wA = a ? a->invMass : 0.0f;
            float iA = a ? a->invInertia : 0.0f;
            float wB = b.invMass;
            float iB = b.invInertia;

            Vec2 vA = a ? (a->vel + Vec2::cross(a->angVel, c.rA)) : Vec2{0.0f, 0.0f};
            Vec2 vB = b.vel + Vec2::cross(b.angVel, c.rB);
            float vn = Vec2::dot(vB - vA, c.normal);

            if (c.jnAccum == 0.0f && c.idA >= 0 && vn < -0.35f) {
                c.restitutionVel = std::max(c.restitutionVel, -c.restitution * vn);
            }

            float dJn = (-vn + c.restitutionVel) * c.effMassN;
            float newJn = std::max(0.0f, c.jnAccum + dJn);
            dJn = newJn - c.jnAccum;
            c.jnAccum = newJn;

            Vec2 impN = c.normal * dJn;
            if (a && !a->isStatic) {
                a->vel -= impN * wA;
                a->angVel -= Vec2::cross(c.rA, impN) * iA;
            }
            if (!b.isStatic) {
                b.vel += impN * wB;
                b.angVel += Vec2::cross(c.rB, impN) * iB;
            }

            solveFriction(c);
            i += 1;
        }
    }
}

void PhysicsWorld::solvePositionConstraints(int pinnedId) {
    const int POS_ITERS = 8;

    std::vector<size_t> order(bodies.size());
    std::iota(order.begin(), order.end(), 0);

    for (int iter = 0; iter < POS_ITERS; ++iter) {
        std::sort(order.begin(), order.end(), [&](size_t i, size_t j) {
            return bodies[i].pos.y < bodies[j].pos.y;
        });

        std::vector<bool> grounded(bodies.size(), false);
        for (size_t i = 0; i < bodies.size(); ++i) {
            if (bodies[i].isStatic || (int)i == pinnedId) grounded[i] = true;
        }

        for (size_t idx : order) {
            Body& b = bodies[idx];
            if (b.isStatic) continue;

            if (b.shape == ShapeType::CIRCLE) {
                if (b.pos.y - b.radius <= groundY + 1e-4f) {
                    b.pos.y = std::max(b.pos.y, groundY + b.radius);
                    grounded[b.id] = true;
                }
            } else {
                auto verts = b.getVertices();
                float minY = std::min({verts[0].y, verts[1].y, verts[2].y, verts[3].y});
                if (minY <= groundY + 1e-4f) {
                    if (minY < groundY) b.pos.y += (groundY - minY);
                    grounded[b.id] = true;

                    if (std::abs(b.angle) < 0.035f && std::abs(b.angVel) < 0.8f) {
                        b.angle = 0.0f;
                        b.angVel = 0.0f;
                        b.pos.y = groundY + b.height * 0.5f;
                    }
                }
            }
        }

        for (size_t ii = 0; ii < order.size(); ++ii) {
            for (size_t jj = ii + 1; jj < order.size(); ++jj) {
                Body& a = bodies[order[ii]];
                Body& b = bodies[order[jj]];
                if (a.isStatic && b.isStatic) continue;

                OBBManifold m = CollideBodies(a, b, 0.0f);
                if (!m.hit || m.depth <= 1e-5f) continue;

                float wA = (a.isStatic || a.id == pinnedId) ? 0.0f : a.invMass;
                float wB = (b.isStatic || b.id == pinnedId) ? 0.0f : b.invMass;

                if (m.normal.y > 0.5f && grounded[a.id] && !b.isStatic && b.id != pinnedId) {
                    wA = 0.0f;
                } else if (m.normal.y < -0.5f && grounded[b.id] && !a.isStatic && a.id != pinnedId) {
                    wB = 0.0f;
                }

                float totalW = wA + wB;
                if (totalW <= 1e-6f) continue;

                float factor = (wA == 0.0f || wB == 0.0f) ? 1.0f : 0.85f;
                Vec2 corr = m.normal * (m.depth * factor / totalW);
                a.pos -= corr * wA;
                b.pos += corr * wB;

                if (m.normal.y > 0.5f && grounded[a.id]) {
                    grounded[b.id] = true;
                    if (a.shape == ShapeType::BOX && b.shape == ShapeType::BOX) {
                        float diffAngle = b.angle - a.angle;
                        if (std::abs(diffAngle) < 0.035f && std::abs(b.angVel) < 0.8f) {
                            b.angle = a.angle;
                            b.angVel = 0.0f;
                        }
                    }
                } else if (m.normal.y < -0.5f && grounded[b.id]) {
                    grounded[a.id] = true;
                    if (a.shape == ShapeType::BOX && b.shape == ShapeType::BOX) {
                        float diffAngle = a.angle - b.angle;
                        if (std::abs(diffAngle) < 0.035f && std::abs(a.angVel) < 0.8f) {
                            a.angle = b.angle;
                            a.angVel = 0.0f;
                        }
                    }
                }
            }
        }

        for (auto& r : ropes) {
            if (r.idB == -1) continue;
            Body& a = bodies[r.idA];
            Body& b = bodies[r.idB];

            Vec2 pA = a.localToWorld(r.offsetA);
            Vec2 pB = b.localToWorld(r.offsetB);
            Vec2 delta = pB - pA;
            float dist = delta.length();
            if (dist <= 1e-6f) continue;

            float maxLen = r.isRigid ? r.restLength : (r.restLength + r.maxStretch);
            float minLen = r.isSpring ? (r.isRigid ? r.restLength : r.restLength * 0.25f) : 0.0f;

            if (dist > maxLen) {
                float diff = dist - maxLen;
                float totalW = a.invMass + b.invMass;
                if (totalW > 1e-6f) {
                    Vec2 n = delta / dist;
                    Vec2 corr = n * (diff / totalW);
                    if (!a.isStatic) a.pos += corr * a.invMass;
                    if (!b.isStatic) b.pos -= corr * b.invMass;
                }
                r.currentLength = maxLen;
            } else if (dist < minLen) {
                float diff = minLen - dist;
                float totalW = a.invMass + b.invMass;
                if (totalW > 1e-6f) {
                    Vec2 n = delta / dist;
                    Vec2 corr = n * (diff / totalW);
                    if (!a.isStatic) a.pos -= corr * a.invMass;
                    if (!b.isStatic) b.pos += corr * b.invMass;
                }
                r.currentLength = minLen;
            } else {
                r.currentLength = dist;
            }
            r.dir = (a.localToWorld(r.offsetA) - b.localToWorld(r.offsetB)).normalized();
        }
    }
}

void PhysicsWorld::enforceBoundaries() {
    for (auto& b : bodies) {
        if (b.isStatic) continue;

        float minY;
        if (b.shape == ShapeType::CIRCLE) {
            minY = b.pos.y - b.radius;
        } else {
            auto verts = b.getVertices();
            minY = std::min({verts[0].y, verts[1].y, verts[2].y, verts[3].y});
        }

        if (minY < groundY) {
            b.pos.y += (groundY - minY);
            if (b.vel.y < -2.2f) b.vel.y = -b.vel.y * b.restitution;
            else if (b.vel.y < 0.0f) b.vel.y = 0.0f;
        }
    }
}

void PhysicsWorld::updateStaticTelemetry() {
    for (auto& b : bodies) {
        b.supportedBy = -1;
        b.normalForce = 0.0f;
        b.topLoadForce = 0.0f;
        b.frictionForce = 0.0f;
    }

    const float EPS = 0.04f;
    for (auto& b : bodies) {
        if (b.isStatic) continue;

        float minY;
        if (b.shape == ShapeType::CIRCLE) {
            minY = b.pos.y - b.radius;
        } else {
            auto verts = b.getVertices();
            minY = std::min({verts[0].y, verts[1].y, verts[2].y, verts[3].y});
        }

        if (minY <= groundY + EPS) {
            b.supportedBy = -2;
            continue;
        }

        for (const auto& other : bodies) {
            if (other.id == b.id || other.isStatic) continue;
            OBBManifold m = CollideBodies(other, b, EPS);
            if (m.hit && m.normal.y > 0.5f) {
                b.supportedBy = other.id;
                break;
            }
        }
    }

    std::vector<int> order;
    for (const auto& b : bodies) {
        if (!b.isStatic) order.push_back(b.id);
    }
    std::sort(order.begin(), order.end(), [&](int i, int j) {
        return bodies[i].pos.y > bodies[j].pos.y;
    });

    for (auto& r : ropes) {
        Vec2 pA = bodies[r.idA].localToWorld(r.offsetA);
        if (r.idB == -1) {
            Vec2 delta = r.freePos - pA;
            r.currentLength = delta.length();
            r.dir = (r.currentLength > 1e-6f) ? ((pA - r.freePos) / r.currentLength) : Vec2{0.0f, 1.0f};
            r.tension = 0.0f;
        } else {
            Vec2 pB = bodies[r.idB].localToWorld(r.offsetB);
            Vec2 delta = pB - pA;
            r.currentLength = delta.length();
            r.dir = (r.currentLength > 1e-6f) ? ((pA - pB) / r.currentLength) : Vec2{0.0f, 1.0f};
            r.tension = (!r.isRigid && r.isSpring) ? (r.stiffness * r.extension()) : 0.0f;
        }
    }

    for (int id : order) {
        Body& b = bodies[id];
        float totalDown = b.mass * G + b.topLoadForce;
        bool held = false;

        for (auto& r : ropes) {
            if (r.idB == id && r.currentLength >= r.restLength - 0.02f && b.supportedBy != -2) {
                r.tension = totalDown;
                held = true;
            }
        }
        if (!held) {
            if (b.supportedBy == -2) {
                b.normalForce = totalDown;
            } else if (b.supportedBy >= 0) {
                b.normalForce = totalDown;
                bodies[b.supportedBy].topLoadForce += totalDown;
            }
            if (std::abs(b.appliedForceX) > 0.01f) {
                b.frictionForce = std::min(std::abs(b.appliedForceX), b.friction * b.normalForce);
            }
        }
    }
}