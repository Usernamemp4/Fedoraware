#include "ProjectileAimbot.h"

void CProjectileAimbot::FillWeaponInfo() {
	const int& iCurWeapon = G::CurItemDefIndex;	//	this reference looks nicer than writing out CurItemDefIndex every time.

}

float CProjectileAimbot::GetInterpolatedStartPosOffset()
{
	return G::LerpTime * iWeapon.flInitialVelocity;
}


int CProjectileAimbot::GetTargetIndex() {
	CBaseEntity* pLocal = g_EntityCache.GetLocal();
	const Vec3 vAngles = pLocal->GetEyeAngles();
	const Vec3 vEyePos = pLocal->GetEyePosition();

	for (CBaseEntity* CTFPlayer : g_EntityCache.GetGroup(CanTargetTeammates() ? EGroupType::PLAYERS_ALL : EGroupType::PLAYERS_ENEMIES)) {
		//	Lets do an FoV check first.
		float flFoVTo = -1;
		for (int iBone = 0; iBone < 15; iBone++) {
			const Vec3 vBonePos = CTFPlayer->GetHitboxPos(iBone);
			const Vec3 vAngTo = Math::CalcAngle(vEyePos, vBonePos);
			const float flFoVToTemp = Math::CalcFov(vAngles, vAngTo);
			if (flFoVToTemp < flFoVTo && flFoVToTemp < Vars::Aimbot::Global::AimFOV.Value) {
				flFoVTo = flFoVToTemp;
			}
		}
		if (flFoVTo < 0) { continue; }

		if ((CTFPlayer->GetTeamNum() == pLocal->GetTeamNum())) {
			if (CTFPlayer->GetHealth() >= CTFPlayer->GetMaxHealth())
			{ continue; }
		}
		else if (F::AimbotGlobal.ShouldIgnore(CTFPlayer)) {
			continue;
		}


	}
}

//	Scans points and returns the most appropriate one.
Vec3 CProjectileAimbot::GetPoint(const int nHitbox)
{
	CBaseEntity* pLocal = g_EntityCache.GetLocal();
	const Vec3 vMaxs = iTargetInfo.vMaxs;
	const Vec3 vMins = iTargetInfo.vMins;
	const Vec3 vBase = iTargetInfo.vAbsOrigin;
}

int CProjectileAimbot::GetHitbox()	//	only for bone aimbot!!!
{
	CBaseEntity* pLocal = g_EntityCache.GetLocal();
	const int iAimMode = Vars::Aimbot::Projectile::AimPosition.Value;
	if (pLocal->GetClassNum() == ETFClass::CLASS_SNIPER && iAimMode == 3) { return 0; }
	return -1;
}

bool CProjectileAimbot::ShouldBounce()
{
	CBaseEntity* pLocal = g_EntityCache.GetLocal();
	return kBounce.Down() ? (pLocal->GetClassNum() == ETFClass::CLASS_SOLDIER || pLocal->GetClassNum() == ETFClass::CLASS_DEMOMAN) : false;
}

bool CProjectileAimbot::CanSeePoint()
{
	CBaseEntity* pLocal = g_EntityCache.GetLocal();
	Utils::VisPosMask(pLocal, iTarget.pEntity, iWeapon.vInitialLocation, iTarget.vShootPos, MASK_SHOT_HULL);
}

//	will do a hull trace
bool CProjectileAimbot::BoxTraceEnd() 
{
	CGameTrace tTrace = {};
	static CTraceFilterWorldAndPropsOnly tTraceFilter = {};
	tTraceFilter.pSkip = iTarget.pEntity;
	Utils::TraceHull(iWeapon.vInitialLocation, iTarget.vShootPos, iWeapon.vMaxs, iWeapon.vMins, MASK_SHOT_HULL, &tTraceFilter, &tTrace);

	return !tTrace.DidHit() && !tTrace.entity;
}

Vec3 CProjectileAimbot::GetHitboxOffset(const Vec3 vBase, const int nHitbox) {
	return GetPointOffset(vBase, iTarget.pEntity->GetHitboxPos(nHitbox));
}

Vec3 CProjectileAimbot::GetPointOffset(const Vec3 vBase, const Vec3 vPoint)
{
	return vPoint - vBase;
}

Vec3 CProjectileAimbot::GetClosestPoint(const Vec3 vCompare)
{
	Vec3 vPos{};
	iTarget.pEntity->GetCollision()->CalcNearestPoint(vCompare, &vPos);
	return vPos;
}
