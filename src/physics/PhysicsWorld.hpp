#pragma once
#include "Body.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

struct RopeLink {
    int idA = -1;
    int idB = -1;
    Vec2 offsetA = {0.0f, 0.0f};
    Vec2 offsetB = {0.0f, 0.0f};
    float restLength = 0.0f;      // Ненагруженная длина L0 (м)
    float currentLength = 0.0f;   // Текущая длина под нагрузкой L (м)
    float stiffness = 220.0f;     // Жесткость нити k (Н/м)
    float tension = 0.0f;         // Сила натяжения T (Н)
    Vec2 dir = {0.0f, 1.0f};      // Направление от B к A

    RopeLink(int a, int b, Vec2 offA, Vec2 offB, float length, float k = 220.0f)
        : idA(a), idB(b), offsetA(offA), offsetB(offB),
          restLength(length), currentLength(length), stiffness(k) {}

    float extension() const {
        return std::max(0.0f, currentLength - restLength);
    }
};

class PhysicsWorld {
public:
    const float G = 9.81f;
    std::vector<Body> bodies;
    std::vector<RopeLink> ropes;
    float groundY = 0.5f;

    int createBody(Vec2 pos, float w, float h, float mass, bool isStatic = false) {
        int id = (int)bodies.size();
        bodies.emplace_back(id, pos, w, h, mass, isStatic);
        return id;
    }

    void addRope(int idA, int idB, Vec2 offA, Vec2 offB, float length, float stiffness = 220.0f) {
        ropes.emplace_back(idA, idB, offA, offB, length, stiffness);
    }

    void step(float dt, int draggedId = -1) {
        const int SUBSTEPS = 20;
        float subDt = dt / (float)SUBSTEPS;

        for (int s = 0; s < SUBSTEPS; ++s) {
            // 1. Интегрирование скоростей и координат (только для свободных тел)
            for (auto& b : bodies) {
                if (!b.isStatic && b.id != draggedId) {
                    b.vel.y -= G * subDt;
                    b.pos += b.vel * subDt;
                }
            }

            // 2. Упругая нерастяжимая нить (XPBD + закон Гука)
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

                float extraTopMass = b.topLoadForce / G;
                float effectiveMassB = b.mass + extraTopMass;
                float invMassB = (b.isStatic || b.id == draggedId) ? 0.0f : (1.0f / effectiveMassB);
                float invMassA = (a.isStatic || a.id == draggedId) ? 0.0f : a.invMass;
                float totalW = invMassA + invMassB;

                if (b.id == draggedId) {
                    float maxDragLen = r.restLength + (effectiveMassB * G) / r.stiffness + 0.35f;
                    if (dist > maxDragLen) {
                        Vec2 n = delta / dist;
                        b.pos = pA + n * maxDragLen - r.offsetB;
                        r.currentLength = maxDragLen;
                    }
                    continue;
                }

                float diff = dist - r.restLength;
                if (diff <= 0.0f || totalW <= 1e-6f) continue;

                Vec2 n = delta / dist;
                float compliance = 1.0f / (r.stiffness * subDt * subDt);
                float dLambda = diff / (totalW + compliance);

                Vec2 corr = n * dLambda;
                a.pos += corr * invMassA;
                b.pos -= corr * invMassB;

                Vec2 velImpulse = corr / subDt;
                a.vel += velImpulse * invMassA;
                b.vel -= velImpulse * invMassB;

                Vec2 vRel = b.vel - a.vel;
                float vn = Vec2::dot(vRel, n);
                float radialDamping = std::min(1.0f, 12.0f * subDt);
                Vec2 dampImpulse = n * (vn * radialDamping / totalW);
                a.vel += dampImpulse * invMassA;
                b.vel -= dampImpulse * invMassB;
            }

            // 3. Коллизии блоков друг с другом
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

                    if (px < py) {
                        float sign = (dx > 0.0f) ? 1.0f : -1.0f;
                        Vec2 corr = {sign * px / totalW, 0.0f};
                        a.pos -= corr * wA;
                        b.pos += corr * wB;
                        float vxAvg = (a.vel.x * a.mass + b.vel.x * b.mass) / (a.mass + b.mass);
                        if (!a.isStatic && a.id != draggedId) a.vel.x = vxAvg;
                        if (!b.isStatic && b.id != draggedId) b.vel.x = vxAvg;
                    } else {
                        float sign = (dy > 0.0f) ? 1.0f : -1.0f;
                        Vec2 corr = {0.0f, sign * py / totalW};
                        a.pos -= corr * wA;
                        b.pos += corr * wB;

                        float vyAvg = (a.vel.y * a.mass + b.vel.y * b.mass) / (a.mass + b.mass);
                        if (sign > 0.0f && b.vel.y < a.vel.y) {
                            if (!a.isStatic && a.id != draggedId) a.vel.y = vyAvg;
                            if (!b.isStatic && b.id != draggedId) b.vel.y = vyAvg;
                            if (!b.isStatic && b.id != draggedId) {
                                b.vel.x = a.vel.x;
                                b.pos.x += a.vel.x * subDt;
                            }
                        } else if (sign < 0.0f && a.vel.y < b.vel.y) {
                            if (!a.isStatic && a.id != draggedId) a.vel.y = vyAvg;
                            if (!b.isStatic && b.id != draggedId) b.vel.y = vyAvg;
                            if (!a.isStatic && a.id != draggedId) {
                                a.vel.x = b.vel.x;
                                a.pos.x += b.vel.x * subDt;
                            }
                        }
                    }
                }
            }

            // 4. Коллизия с полом
            for (auto& b : bodies) {
                if (!b.isStatic) {
                    float bottom = b.pos.y - b.height * 0.5f;
                    if (bottom < groundY) {
                        b.pos.y = groundY + b.height * 0.5f;
                        if (b.vel.y < 0.0f) b.vel.y = 0.0f;
                        if (b.id != draggedId) {
                            b.vel.x *= (1.0f - 5.0f * subDt);
                        }
                    }
                }
            }
        }

        // Минимальное естественное сопротивление воздуха (исправленная строка)
        for (auto& b : bodies) {
            if (!b.isStatic && b.id != draggedId) {
                b.vel = b.vel * (1.0f - 0.04f * dt);
            }
        }

        updateForces();
    }

    void updateForces() {
        for (auto& b : bodies) {
            b.supportedBy = -1;
            b.normalForce = 0.0f;
            b.topLoadForce = 0.0f;
        }

        const float EPS = 0.05f;

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

        for (auto& r : ropes) r.tension = 0.0f;

        for (int id : order) {
            Body& b = bodies[id];
            float totalMass = b.mass + (b.topLoadForce / G);
            float totalDownward = totalMass * G;

            bool held = false;
            for (auto& r : ropes) {
                if (r.idB == id) {
                    Vec2 pA = bodies[r.idA].pos + r.offsetA;
                    Vec2 pB = b.pos + r.offsetB;
                    Vec2 delta = pB - pA;
                    float dist = delta.length();
                    r.currentLength = dist;

                    if (dist >= r.restLength - 0.01f && b.supportedBy != -2) {
                        r.dir = (pA - pB).normalized();
                        Vec2 tangent = {-r.dir.y, r.dir.x};
                        float vTangential = Vec2::dot(b.vel, tangent);
                        float centripetalForce = (dist > 0.1f) ? (totalMass * vTangential * vTangential / dist) : 0.0f;
                        float cosAlpha = Vec2::dot(r.dir, {0.0f, 1.0f});
                        float gravityProj = totalDownward * cosAlpha;

                        r.tension = std::max(0.0f, gravityProj + centripetalForce);
                        held = true;
                    }
                }
            }

            if (!held) {
                if (b.supportedBy == -2) {
                    b.normalForce = totalDownward;
                } else if (b.supportedBy >= 0) {
                    b.normalForce = totalDownward;
                    bodies[b.supportedBy].topLoadForce += totalDownward;
                }
            }
        }
    }
};