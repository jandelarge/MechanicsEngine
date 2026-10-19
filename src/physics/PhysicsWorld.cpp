#include "PhysicsWorld.hpp"
#include <cmath>
#include <algorithm>

int PhysicsWorld::createBody(Vec2 pos, float w, float h, float mass, bool isStatic) {
    int id = (int)bodies.size();
    bodies.emplace_back(id, pos, w, h, mass, isStatic);
    return id;
}

int PhysicsWorld::addRope(int idA, int idB, Vec2 offA, Vec2 offB, float length, float stiffness) {
    int id = (int)ropes.size();
    ropes.emplace_back(id, idA, idB, offA, offB, length, stiffness);
    ropes.back().damping = 20.0f;
    return id;
}

void PhysicsWorld::resetDefaultScene(int hangingId, int freeId) {
    if (hangingId >= 0 && hangingId < (int)bodies.size()) {
        bodies[hangingId].pos = {0.0f, 4.0f};
        bodies[hangingId].vel = {0.0f, 0.0f};
    }
    if (freeId >= 0 && freeId < (int)bodies.size()) {
        bodies[freeId].pos = {-3.2f, 1.0f};
        bodies[freeId].vel = {0.0f, 0.0f};
    }
    updateForces();
}

void PhysicsWorld::step(float dt, int draggedId, const Vec2& targetBodyPos) {
    const int SUBSTEPS = 24;
    float subDt = dt / (float)SUBSTEPS;

    for (int s = 0; s < SUBSTEPS; ++s) {
        // 1. Сначала прикладываем силы (гравитация, сопротивление воздуха и натяжение тросов) к скоростям
        for (auto& b : bodies) {
            if (b.isStatic || b.id == draggedId) continue;

            b.vel.y -= G * subDt;

            float speed = b.vel.length();
            float dragFactor = 0.15f + 0.04f * speed;
            b.vel *= std::max(0.0f, 1.0f - dragFactor * subDt);
        }

        solveRopes(subDt, draggedId);

        // 2. Двигаем координаты (захваченный блок идет 1:1 к курсору без отставания)
        for (auto& b : bodies) {
            if (b.isStatic) continue;

            if (b.id == draggedId) {
                int remaining = SUBSTEPS - s;
                Vec2 stepMove = (targetBodyPos - b.pos) / (float)remaining;
                b.vel = stepMove / subDt;
                b.pos += stepMove;
            } else {
                b.pos += b.vel * subDt;
            }
        }

        // 3. Разрешаем коллизии и сухое трение между блоками
        solveCollisions(subDt, draggedId);

        // 4. Жесткие границы экрана, пол и предел прочности троса
        enforceHardLimits(draggedId);
    }

    updateForces();
}

void PhysicsWorld::solveRopes(float subDt, int draggedId) {
    for (auto& r : ropes) {
        Body& a = bodies[r.idA];
        Body& b = bodies[r.idB];

        Vec2 pA = a.pos + r.offsetA;
        Vec2 pB = b.pos + r.offsetB;
        Vec2 delta = pB - pA;
        float dist = delta.length();
        if (dist <= 1e-6f) continue;

        r.dir = (pA - pB) / dist;
        r.currentLength = dist;

        float ext = dist - r.restLength;
        if (ext <= 0.0f) {
            r.tension = 0.0f;
            continue;
        }

        Vec2 n = delta / dist;
        Vec2 vRel = b.vel - a.vel;
        float vn = Vec2::dot(vRel, n);

        float springForce = r.stiffness * ext;
        float dampForce = r.damping * vn;
        float totalRopeForce = std::max(0.0f, springForce + dampForce);

        r.tension = springForce;

        Vec2 forceVec = n * totalRopeForce;
        if (!a.isStatic && a.id != draggedId) {
            a.vel += forceVec * (a.invMass * subDt);
        }
        if (!b.isStatic && b.id != draggedId) {
            b.vel -= forceVec * (b.invMass * subDt);
        }
    }
}

void PhysicsWorld::solveCollisions(float subDt, int draggedId) {
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            Body& a = bodies[i];
            Body& b = bodies[j];
            if (a.isStatic && b.isStatic) continue;

            float dx = b.pos.x - a.pos.x;
            float px = (a.width + b.width) * 0.5f - std::abs(dx);
            if (px <= 0.0f) continue;

            float dy = b.pos.y - a.pos.y;
            float py = (a.height + b.height) * 0.5f - std::abs(dy);
            if (py <= 0.0f) continue;

            float wA = (a.isStatic || a.id == draggedId) ? 0.0f : a.invMass;
            float wB = (b.isStatic || b.id == draggedId) ? 0.0f : b.invMass;
            float totalW = wA + wB;
            if (totalW <= 1e-6f) continue;

            Vec2 normal = {0.0f, 0.0f};
            float penetration = 0.0f;

            if (px < py) {
                normal = {(dx > 0.0f) ? 1.0f : -1.0f, 0.0f};
                penetration = px;
            } else {
                normal = {0.0f, (dy > 0.0f) ? 1.0f : -1.0f};
                penetration = py;
            }

            // Выталкивание вдоль нормали
            Vec2 corr = normal * (penetration / totalW);
            a.pos -= corr * wA;
            b.pos += corr * wB;

            // Нормальный импульс реакции опоры
            Vec2 vRel = b.vel - a.vel;
            float vn = Vec2::dot(vRel, normal);
            float jn = 0.0f;

            if (vn < 0.0f) {
                jn = -vn / totalW;
                Vec2 impN = normal * jn;
                a.vel -= impN * wA;
                b.vel += impN * wB;
            }

            // Сухое трение Кулона (удерживает верхний блок от скольжения по нижнему)
            Vec2 tangent = {-normal.y, normal.x};
            float vt = Vec2::dot(b.vel - a.vel, tangent);
            if (std::abs(vt) > 1e-6f) {
                float jt = -vt / totalW;

                float minMass = std::min(a.mass, b.mass);
                float normalLoad = std::max(jn, minMass * G * subDt);
                float mu = std::min(a.friction, b.friction);
                float maxJt = mu * normalLoad * 1.5f;

                // Если сила сдвига меньше порога трения покоя — убираем микроскольжение координат
                if (std::abs(jt) <= maxJt) {
                    float slip = vt * subDt;
                    a.pos += tangent * (slip * (wA / totalW));
                    b.pos -= tangent * (slip * (wB / totalW));
                }

                jt = std::clamp(jt, -maxJt, maxJt);
                Vec2 impT = tangent * jt;
                a.vel -= impT * wA;
                b.vel += impT * wB;
            }
        }
    }
}

void PhysicsWorld::enforceHardLimits(int draggedId) {
    // 1. Границы экрана и пол
    for (auto& b : bodies) {
        if (b.isStatic) continue;

        float halfW = b.width * 0.5f;
        float halfH = b.height * 0.5f;

        if (b.pos.y - halfH < groundY) {
            b.pos.y = groundY + halfH;
            if (b.vel.y < 0.0f) b.vel.y = 0.0f;
            if (b.id != draggedId) b.vel.x *= 0.90f;
        }
        if (b.pos.y + halfH > worldTopY) {
            b.pos.y = worldTopY - halfH;
            if (b.vel.y > 0.0f) b.vel.y = 0.0f;
        }
        if (b.pos.x - halfW < -worldHalfWidth) {
            b.pos.x = -worldHalfWidth + halfW;
            if (b.vel.x < 0.0f) b.vel.x = 0.0f;
        }
        if (b.pos.x + halfW > worldHalfWidth) {
            b.pos.x = worldHalfWidth - halfW;
            if (b.vel.x > 0.0f) b.vel.x = 0.0f;
        }
    }

    // 2. Жесткий предел растяжения троса
    for (auto& r : ropes) {
        Body& a = bodies[r.idA];
        Body& b = bodies[r.idB];
        Vec2 pA = a.pos + r.offsetA;
        Vec2 pB = b.pos + r.offsetB;
        Vec2 delta = pB - pA;
        float dist = delta.length();
        float maxAllowed = r.restLength + r.maxStretch;

        if (dist > maxAllowed && dist > 1e-6f) {
            Vec2 n = delta / dist;
            if (!b.isStatic) {
                b.pos = pA + n * maxAllowed - r.offsetB;
                float vn = Vec2::dot(b.vel, n);
                if (vn > 0.0f) b.vel -= n * vn;
            }
            r.currentLength = maxAllowed;
            r.tension = r.stiffness * r.maxStretch;
        }
    }

    // 3. Выталкивание захваченного мышью тела из других тел
    if (draggedId >= 0 && draggedId < (int)bodies.size()) {
        Body& db = bodies[draggedId];
        for (auto& other : bodies) {
            if (other.id == db.id) continue;

            float dx = db.pos.x - other.pos.x;
            float px = (db.width + other.width) * 0.5f - std::abs(dx);
            if (px <= 0.0f) continue;

            float dy = db.pos.y - other.pos.y;
            float py = (db.height + other.height) * 0.5f - std::abs(dy);
            if (py <= 0.0f) continue;

            if (px < py) {
                db.pos.x += (dx > 0.0f ? px : -px);
                db.vel.x = 0.0f;
            } else {
                db.pos.y += (dy > 0.0f ? py : -py);
                db.vel.y = 0.0f;
            }
        }
    }
}

void PhysicsWorld::updateForces() {
    for (auto& b : bodies) {
        b.supportedBy = -1;
        b.normalForce = 0.0f;
        b.topLoadForce = 0.0f;
    }

    const float EPS = 0.06f;

    for (auto& b : bodies) {
        if (b.isStatic) continue;

        if (b.pos.y - b.height * 0.5f <= groundY + EPS) {
            b.supportedBy = -2;
            continue;
        }

        for (const auto& other : bodies) {
            if (other.id == b.id || other.isStatic) continue;
            float overlapX = (b.width + other.width) * 0.5f - std::abs(b.pos.x - other.pos.x);
            float gapY = (b.pos.y - b.height * 0.5f) - (other.pos.y + other.height * 0.5f);
            if (overlapX > 0.05f && std::abs(gapY) <= EPS) {
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

    for (int id : order) {
        Body& b = bodies[id];
        float totalDownward = b.mass * G + b.topLoadForce;

        bool heldByRope = false;
        for (const auto& r : ropes) {
            if (r.idB == id && r.tension > 0.5f && b.supportedBy != -2) {
                heldByRope = true;
                break;
            }
        }

        if (!heldByRope) {
            if (b.supportedBy == -2) {
                b.normalForce = totalDownward;
            } else if (b.supportedBy >= 0) {
                b.normalForce = totalDownward;
                bodies[b.supportedBy].topLoadForce += totalDownward;
            }
        }
    }
}