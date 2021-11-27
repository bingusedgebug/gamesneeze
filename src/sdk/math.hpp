#pragma once
#include "sdk.hpp"

inline QAngle originalAngle;
inline float originalForwardMove, originalSideMove;

inline void startMovementFix(CUserCmd* cmd) {
    originalAngle = cmd->viewangles;
    originalForwardMove = cmd->forwardmove;
    originalSideMove = cmd->sidemove;
}

inline void endMovementFix(CUserCmd* cmd) {
    // this was just taken from designer bc im lazy
    // https://github.com/designer1337/csgo-cheat-base/blob/09fa2ba8de52eef482bbc82f682976e369191077/dependencies/math/math.cpp#L4
    float deltaViewAngles;
	float f1;
	float f2;

	if (originalAngle.y < 0.f)
		f1 = 360.0f + originalAngle.y;
	else
		f1 = originalAngle.y;

	if (cmd->viewangles.y < 0.0f)
		f2 = 360.0f + cmd->viewangles.y;
	else
		f2 = cmd->viewangles.y;

	if (f2 < f1)
		deltaViewAngles = abs(f2 - f1);
	else
		deltaViewAngles = 360.0f - abs(f1 - f2);

	deltaViewAngles = 360.0f - deltaViewAngles;

	cmd->forwardmove = cos(DEG2RAD(deltaViewAngles)) * originalForwardMove + cos(DEG2RAD(deltaViewAngles + 90.f)) * originalSideMove;
	cmd->sidemove = sin(DEG2RAD(deltaViewAngles)) * originalForwardMove + sin(DEG2RAD(deltaViewAngles + 90.f)) * originalSideMove;
    // TODO: support upmove
}

inline void normalizeAngles(QAngle& angle) {
	while (angle.x > 89.0f)
		angle.x -= 180.f;

	while (angle.x < -89.0f)
		angle.x += 180.f;

	while (angle.y > 180.f)
		angle.y -= 360.f;

	while (angle.y < -180.f)
		angle.y += 360.f;
}

inline void sanitizeAngles(QAngle& angle) {
    normalizeAngles(angle);
    angle.x = std::clamp(angle.x, -89.0f, 89.0f);
    angle.y = std::clamp(angle.y, -180.0f, 180.0f);
    angle.z = 0.0f;
}

inline QAngle calcAngle(const Vector& src, const Vector& dst) {
	QAngle vAngle;
	Vector delta((src.x - dst.x), (src.y - dst.y), (src.z - dst.z));
	double hyp = sqrt(delta.x*delta.x + delta.y*delta.y);

	vAngle.x = float(atanf(float(delta.z / hyp)) * 57.295779513082f);
	vAngle.y = float(atanf(float(delta.y / delta.x)) * 57.295779513082f);
	vAngle.z = 0.0f;

	if (delta.x >= 0.0)
		vAngle.y += 180.0f;

	return vAngle;
}

inline void angleVectors(const QAngle &angles, Vector& forward) {
	forward.x = cos(DEG2RAD(angles.x)) * cos(DEG2RAD(angles.y));
	forward.y = cos(DEG2RAD(angles.x)) * sin(DEG2RAD(angles.y));
	forward.z = -sin(DEG2RAD(angles.x));
}

inline void vectorAngles(const Vector& vec, QAngle& angles) {
    float pitch, yaw;
    if (!(vec.x || vec.y)) {
        pitch = vec.z > 0 ? 270.f : 90.f;
        yaw = 0.f;
    } else {
        pitch = atan2f(-vec.z, vec.Length2D()) * 180.f / M_PI;
        if (pitch < 0.f) pitch += 360.f;
        yaw = atan2f(vec.y, vec.x) * 180.f / M_PI;
        if (yaw < 0.f) yaw += 360.f;
    }
    angles.x = pitch;
    angles.y = yaw;
    angles.z = 0.f;
}

inline float getDistance(Vector pos1, Vector pos2) {
    // Do 3d pythag
    float a = abs(pos1.x-pos2.x);
    float b = abs(pos1.y-pos2.y);
    float c = abs(pos1.z-pos2.z);
    return sqrt(pow(a, 2.f) + pow(b, 2.f) + pow(c, 2.f));
}

inline float getDistanceNoSqrt(Vector pos1, Vector pos2) {
    // When you dont need an exact distance and just want to see if 
	// something is x further than something else for example theres no need to sqrt it
    float a = abs(pos1.x-pos2.x);
    float b = abs(pos1.y-pos2.y);
    float c = abs(pos1.z-pos2.z);
    return pow(a, 2.f) + pow(b, 2.f) + pow(c, 2.f);
}

inline Vector2D anglePixels(QAngle angDelta) {
	static ConVar* sensitivity = Interfaces::convar->FindVar("sensitivity");
	static ConVar* m_yaw = Interfaces::convar->FindVar("m_yaw");
	static ConVar* m_pitch = Interfaces::convar->FindVar("m_pitch");

	sanitizeAngles(angDelta);

	float pixelMovePitch = (-angDelta.x) / (m_pitch->GetFloat() * sensitivity->GetFloat());
	float pixelMoveYaw = (angDelta.y) / (m_yaw->GetFloat() * sensitivity->GetFloat());

	return Vector2D(pixelMoveYaw, pixelMovePitch);
}

inline Vector2D anglePixels(QAngle angBegin, QAngle angEnd) {
	return anglePixels(angBegin - angEnd);
}

inline QAngle pixelAngles(Vector2D vecPixels) {
	static ConVar* sensitivity = Interfaces::convar->FindVar("sensitivity");
	static ConVar* m_yaw = Interfaces::convar->FindVar("m_yaw");
	static ConVar* m_pitch = Interfaces::convar->FindVar("m_pitch");

	float pitch = (-vecPixels.y) * (m_pitch->GetFloat() * sensitivity->GetFloat());
	float yaw = (vecPixels.x) * (m_yaw->GetFloat() * sensitivity->GetFloat());

	return QAngle(pitch, yaw, 0.f);
}

inline QAngle pixelAngles(float x, float y) {
	return pixelAngles(Vector2D(x, y));
}

bool worldToScreen(const Vector& origin, Vector& screen);