#include "ProjectileAimbot.h"

/// 1.0 Scan targets for most viable target.
/// 1.1 Ensure target is visible unless the user has specified that they want to predict non visible entities
/// 1.2 Scan along path until target is visible in a set amount of points (Possibly user specified) before we attempt to aim at them
/// 
/// 2.0 Once we have a verified target, gather required info.
/// 2.1 Gather info about the target & the weapon we are using (if not cached)
/// 
/// 3.0 Create a vector of points that we could use to hit the target
/// 3.1 Validate these points against the selected hitboxes that the user wishes to target
/// 3.2 Check visibility of points prior to seeing if we could hit them with a hull trace
/// 
/// 4.0 Scan all (necessary) available points in the vector until we find a point which we are able to hit also ensure the time matches up for the simtime & travel time
/// 4.1 If the weapon is effected by gravity, instead of a hull trace, arc prediction will be used to ensure we can hit the point.
/// 4.2 Adjust aim as required to hit the target
/// 
/// 5.0 Aim at point
/// 5.1 Fire

/// CProjectileAimbot::FillWeaponInfo
/// This will run ONLY if G::CurItemDefIndex is not the same across two different instances (very minor optimisation imo)
/// It will cache all required data for the aimbot to run
void CProjectileAimbot::FillWeaponInfo() {
	CBaseEntity* pLocal = g_EntityCache.GetLocal();
	const int& iCurWeapon = G::CurItemDefIndex;	//	this reference looks nicer than writing out CurItemDefIndex every time.
	iWeapon.flInitialVelocity = GetWeaponVelocity();
	iWeapon.vInitialLocation = iTarget.vAngles + GetFireOffset();
	iWeapon.vMaxs = GetWeaponBounds(); iWeapon.vMins = iWeapon.vMaxs * -1.f;
}

float CProjectileAimbot::GetInterpolatedStartPosOffset()
{
	return G::LerpTime * iWeapon.flInitialVelocity;
}

bool CProjectileAimbot::DoesTargetNeedPrediction() {
	return iTargetInfo.pEntity->GetVelocity().Length() > 0;	//	buildings can't move so this shouldn't predict buildings :troller:
}

bool CProjectileAimbot::BeginTargetPrediction() {
	iTargetInfo.vBackupAbsOrigin = iTargetInfo.pEntity->GetAbsOrigin();
	return F::MoveSim.Initialize(iTargetInfo.pEntity);
}

void CProjectileAimbot::RunPrediction() {
	F::MoveSim.RunTick(iTargetInfo.iMoveData, iTargetInfo.vAbsOrigin);
	iTargetInfo.pEntity->SetAbsOrigin(iTargetInfo.vAbsOrigin);
}

void CProjectileAimbot::EndTargetPrediction() {
	F::MoveSim.Restore();
	iTargetInfo.pEntity->SetAbsOrigin(iTargetInfo.vBackupAbsOrigin);
}

bool CProjectileAimbot::WillPointHitBone(const Vec3 vPoint, const int nBone, CBaseEntity* pEntity) {
	if (BoneFromPoint(pEntity, vPoint) != nBone) { return false; }
	if (!IsPointVisibleFromPoint(pEntity->GetBonePos(nBone), vPoint)) { return false; }
	return true;
}

int CProjectileAimbot::BoneFromPoint(const Vec3 vPoint, CBaseEntity* pEntity) {
	int iClosestBone = -1;
	float flDistance = FLT_MAX;
	for (int iBone = 0; i <= 14; i++) {
		const Vec3 vBonePos = pEntity->GetBonePos(iBone);
		const float flDelta = vPoint.DistTo(vBonePos);
		if (flDelta < flDistance) {
			iClosestBone = iBone;
			flDistance = flDelta;
		}
	}
	return iBone;
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

	return !tTrace.DidHit() /*&& !tTrace.entity*/;
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
