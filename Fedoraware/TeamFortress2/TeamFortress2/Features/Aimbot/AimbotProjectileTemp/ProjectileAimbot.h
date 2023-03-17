#pragma once
#include "../AimbotGlobal/AimbotGlobal.h"

struct TWeaponInfo {
	Vec3 vMins{};
	Vec3 vMaxs{};	//	vMaxs and vMins will be identical aside from the sign.
	Vec3 vInitialLocation{};	//	the starting location of the projectile
	float flInitialVelocity{};
	float flGravity{};
	float flRange{};
	bool bDrag{};
	bool bSpin{};	//	unlikely to do anything with this as of current.
	int iWeaponID{};
	CBaseCombatWeapon* pWeapon = nullptr;
};

struct TAimInfo {
	Vec3 vAngles{};	//	this will be our angles while we are facing the target
	Vec3 vShootPos{};	//not sure whether to use offset position or not for closest aimbot setting.
	Vec3 vShootOffset{};
	CBaseEntity* pEntity = nullptr;
};

struct TTargetInfo {
	Vec3 vMins{};
	Vec3 vMaxs{};
	Vec3 vAbsOrigin{};
	CBaseEntity* pEntity = nullptr;
};

struct TTargetRecord {
	float flFoVTo{};
	CBaseEntity* pEntity = nullptr;
	//int iType{};
};

class CProjectileAimbot {
private:
	TWeaponInfo iWeapon{};
	TAimInfo iTarget{};
	TTargetInfo iTargetInfo{};

	KeyHelper kBounce{ &Vars::Aimbot::Projectile::BounceKey.Value };

	//	point maths.
	Vec3 GetHitboxOffset(const Vec3 vBase, const int nHitbox);	//	will return a vector to a certain hitbox.
	Vec3 GetPointOffset	(const Vec3 vBase, const Vec3 vPoint);	//	will return a vector to a certain point (for bbox multipoint)
	Vec3 GetClosestPoint(const Vec3 vCompare);

	//	Utils
	//	Info
	void FillWeaponInfo();	//	will use G::CurWeaponIndex to fill weapon info.
	Vec3 GetFireOffset();	//	will get the firing offset of a weapon.	(I have thought about countering the rocket correction done by the game but its pointless)
	float GetInterpolatedStartPosOffset();		//	to make up for interp, projectiles spawn a certain distance behind your weapon
	bool CanTargetTeammates();
	//	Arc Pred
	void RequiredStickyCharge();	//	we should make sure that we can still target enemies with stickies while respecting the max height the sticky can reach, increasing the velocity will reduce the arc required.
	//	Targeting
	std::vector<TTargetRecord> vTargetRecords{};
	Vec3 GetPoint(const int nHitbox);
	int GetHitbox();
	bool ShouldBounce();
	int GetTargetIndex();
	//	Ray Tracing Automatic
	bool CanSeePoint();
	bool BoxTraceEnd();

	//	Prediction
	void PredictProjectileTick();	//	ideally if I add this it will be done differently.
public:
};