#ifdef _WIN32

#define ENGINE_LIB "engine.dll"
#define VPHYSICS_LIB "vphysics.dll"
#define SERVER_LIB "server.dll"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#elif defined _LINUX

#define ENGINE_LIB "engine.so"
#define VPHYSICS_LIB "vphysics.so"
#define SERVER_LIB "garrysmod/bin/server.so"

#endif

#define _RETAIL 1
#define GAME_DLL 1

//hl2sdk-ob-2e2ec01be7aa off the sourcemod mercurial
#include <server/cbase.h>
#include <inetchannel.h>
#include <hl2/vehicle_jeep.h>
#include <public/vphysics_interface.h>

#include "gm_pimpmyride.h"

#include "GMLuaModule.h"

IVEngineServer *engine = NULL;

GMOD_MODULE(Start, Close)

IPhysicsSurfaceProps *surfaceprop;

inline ILuaObject *PushVector( ILuaInterface *gLua, const Vector& vec )
{
	ILuaObject* NVec = gLua->GetGlobal("Vector");

	gLua->Push( NVec );
	gLua->Push( vec.x );
	gLua->Push( vec.y );
	gLua->Push( vec.z );
	gLua->Call( 3, 1 );

	ILuaObject* returno = gLua->GetReturn( 0 );

	NVec->UnReference();

	return returno;
}

IServerUnknown *LookupEntity(int index)
{
	edict_t *pEdict = engine->PEntityOfEntIndex(index);

	if (!pEdict)
		return NULL;

	IServerUnknown *pUnk = pEdict->GetUnknown();

	if (!pUnk)
		return NULL;

	return pUnk;
}

CBaseEntity *GetBaseEntity(int index)
{
	IServerUnknown *pUnknown = LookupEntity(index);
	if(pUnknown == NULL)
		return NULL;

	return pUnknown->GetBaseEntity();
}


#define LUA_GETVEHICLE 	ILuaInterface *gLua = Lua(); \
						IPhysicsVehicleController *vehicle = GetLuaVehicle( gLua ); \
						if(!vehicle) return 0;

IPhysicsVehicleController *GetLuaVehicle(ILuaInterface *gLua)
{
	gLua->CheckType(1, GLua::TYPE_ENTITY);

	CBaseHandle *handle = (CBaseHandle*)gLua->GetUserData(1);
	CBaseEntity *entity = GetBaseEntity(handle->GetEntryIndex());

	if(!entity)
	{
		gLua->Error("[gm_pimpmyride] NO ENTITY!");
		return NULL;
	}

	IServerVehicle *vehicle = entity->GetServerVehicle();

	if(!vehicle)
	{
		gLua->Error("[gm_pimpmyride] NO VEHICLE!");
		return NULL;
	}

	IPhysicsVehicleController *controller = vehicle->GetVehicleController();

	if(!controller)
	{
		gLua->Error("[gm_pimpmyride] NO PHYSICS CONTROLLER!");
		return NULL;
	}

	return controller;
}


LUA_FUNCTION(getoperatingparams)
{
	LUA_GETVEHICLE

	const vehicle_operatingparams_t params = vehicle->GetOperatingParams();

	ILuaObject *table = gLua->GetNewTable();

	table->SetMember("speed", params.speed);
	table->SetMember("RPM", params.engineRPM);
	table->SetMember("gear", (float)params.gear);
	table->SetMember("steeringAngle", params.steeringAngle);
	table->SetMember("wheelsInContact", (float)params.wheelsInContact);
	table->SetMember("isTorqueBoosting", params.isTorqueBoosting);

	gLua->Push(table);

	table->UnReference();

	return 1;
}

// lua_run require("pimpmyride") x = Entity(168):GetVehicleParams() PrintTable(x)
// lua_run Entity(168):SetVehicleParams(x)

LUA_FUNCTION(setvehicleparams)
{
	LUA_GETVEHICLE

	gLua->CheckType(2, GLua::TYPE_TABLE);

	ILuaObject *table = gLua->GetObject(2);

	bool sane = table->GetMemberBool("_SanityCheckOnlyUseTheResultFromGetVehicleParams", false);
	if(!sane)
	{
		gLua->Error("[gm_pimpmyride] USE THE RESULT FROM GETVEHICLEPARAMS");
		return 0;
	}

	vehicleparams_t &params = vehicle->GetVehicleParamsForChange();

	ILuaObject *body = table->GetMember("body");

	Vector *mass = (Vector *)body->GetMemberUserData("massCenterOverride");
	params.body.massCenterOverride.x = mass->x;
	params.body.massCenterOverride.y = mass->y;
	params.body.massCenterOverride.z = mass->z;

	params.body.massOverride = body->GetMemberFloat("massOverride");
	params.body.addGravity = body->GetMemberFloat("addGravity");
	params.body.tiltForce = body->GetMemberFloat("tiltForce");
	params.body.tiltForceHeight = body->GetMemberFloat("tiltForceHeight");
	params.body.counterTorqueFactor = body->GetMemberFloat("counterTorqueFactor");
	params.body.keepUprightTorque = body->GetMemberFloat("keepUprightTorque");
	params.body.maxAngularVelocity = body->GetMemberFloat("maxAngularVelocity");

	ILuaObject *axles = table->GetMember("axles");
	for(int i=0; i < params.axleCount; i++)
	{
		ILuaObject *axle = axles->GetMember((float)i+1);

		params.axles[i].torqueFactor = axle->GetMemberFloat("torqueFactor");
		params.axles[i].brakeFactor = axle->GetMemberFloat("brakeFactor");
		params.axles[i].suspension.springConstant = axle->GetMemberFloat("springConstant");
		params.axles[i].wheels.springAdditionalLength = axle->GetMemberFloat("springAdditionalLength");
		params.axles[i].wheels.frictionScale = axle->GetMemberFloat("frictionScale");

		axle->UnReference();
	}

	ILuaObject *engine = table->GetMember("engine");
	params.engine.horsepower = engine->GetMemberFloat("horsepower");
	params.engine.maxSpeed = engine->GetMemberFloat("maxSpeed");
	params.engine.maxRevSpeed = engine->GetMemberFloat("maxRevSpeed");
	params.engine.maxRPM = engine->GetMemberFloat("maxRPM");
	params.engine.axleRatio = engine->GetMemberFloat("axleRatio");
	params.engine.throttleTime = engine->GetMemberFloat("throttleTime");
	params.engine.gearCount = engine->GetMemberInt("gearCount");
	ILuaObject *gears = engine->GetMember("gearRatio");
	for(int i=0; i < params.engine.gearCount; i++)
	{
		ILuaObject *g = gears->GetMember((float)i+1);

		params.engine.gearRatio[i] = g->GetFloat();
	}
	gears->UnReference();
	params.engine.shiftUpRPM = engine->GetMemberFloat("shiftUpRPM");
	params.engine.shiftDownRPM = engine->GetMemberFloat("shiftDownRPM");
	params.engine.boostForce = engine->GetMemberFloat("boostForce");
	params.engine.boostDuration = engine->GetMemberFloat("boostDuration");
	params.engine.boostDelay = engine->GetMemberFloat("boostDelay");
	params.engine.boostMaxSpeed = engine->GetMemberFloat("boostMaxSpeed");
	params.engine.torqueBoost = engine->GetMemberBool("torqueBoost");

	ILuaObject *steering = table->GetMember("steering");
	params.steering.degreesSlow = steering->GetMemberFloat("degreesSlow");
	params.steering.degreesFast = steering->GetMemberFloat("degreesFast");
	params.steering.degreesBoost = steering->GetMemberFloat("degreesBoost");
	params.steering.speedSlow = steering->GetMemberFloat("speedSlow");
	params.steering.speedFast = steering->GetMemberFloat("speedFast");
	params.steering.powerSlideAccel = steering->GetMemberFloat("powerSlideAccel");
	params.steering.steeringExponent = steering->GetMemberFloat("steeringExponent");
	params.steering.isSkidAllowed = steering->GetMemberBool("isSkidAllowed");
	params.steering.dustCloud = steering->GetMemberBool("dustCloud");
	params.steering.turnThrottleReduceSlow = steering->GetMemberFloat("turnThrottleReduceSlow");
	params.steering.turnThrottleReduceFast = steering->GetMemberFloat("turnThrottleReduceFast");
	params.steering.boostSteeringRateFactor = steering->GetMemberFloat("boostSteeringRateFactor");

	steering->UnReference();
	engine->UnReference();
	axles->UnReference();
	body->UnReference();
	table->UnReference();

	vehicle->VehicleDataReload();

	return 0;
}

LUA_FUNCTION(getvehicleparams)
{
	LUA_GETVEHICLE

	const vehicleparams_t params = vehicle->GetVehicleParams();

	ILuaObject *table = gLua->GetNewTable();

	table->SetMember("_SanityCheckOnlyUseTheResultFromGetVehicleParams", true);

	table->SetMember("axleCount", (float)params.axleCount);
	table->SetMember("wheelsPerAxle", (float)params.wheelsPerAxle);

		ILuaObject *body = gLua->GetNewTable();

		ILuaObject *vec = PushVector(gLua, params.body.massCenterOverride);
		body->SetMember("massCenterOverride", vec);
		body->SetMember("massOverride", params.body.massOverride);
		body->SetMember("addGravity", params.body.addGravity);
		body->SetMember("tiltForce", params.body.tiltForce);
		body->SetMember("tiltForceHeight", params.body.tiltForceHeight);
		body->SetMember("counterTorqueFactor", params.body.counterTorqueFactor);
		body->SetMember("keepUprightTorque", params.body.keepUprightTorque);
		body->SetMember("maxAngularVelocity", params.body.maxAngularVelocity);

	table->SetMember("body", body);

		ILuaObject *axles = gLua->GetNewTable();
		for(int i=0; i < params.axleCount; i++)
		{
			ILuaObject *axle = gLua->GetNewTable();
			axle->SetMember("torqueFactor", params.axles[i].torqueFactor);
			axle->SetMember("brakeFactor", params.axles[i].brakeFactor);
			axle->SetMember("springConstant", params.axles[i].suspension.springConstant);
			axle->SetMember("springAdditionalLength", params.axles[i].wheels.springAdditionalLength);
			axle->SetMember("frictionScale", params.axles[i].wheels.frictionScale);

			axles->SetMember((float)i+1, (ILuaObject *)axle);
			axle->UnReference();
		}

	table->SetMember("axles", axles);

		ILuaObject *engine = gLua->GetNewTable();
		engine->SetMember("horsepower", params.engine.horsepower);
		engine->SetMember("maxSpeed", params.engine.maxSpeed);
		engine->SetMember("maxRevSpeed", params.engine.maxRevSpeed);
		engine->SetMember("maxRPM", params.engine.maxRPM);
		engine->SetMember("axleRatio", params.engine.axleRatio);
		engine->SetMember("throttleTime", params.engine.throttleTime);
		engine->SetMember("gearCount", (float)params.engine.gearCount);
			ILuaObject *gears = gLua->GetNewTable();
			for(int i=0; i < params.engine.gearCount; i++)
			{
				gears->SetMember((float)i+1, (float)params.engine.gearRatio[i]);
			}
		engine->SetMember("gearRatio", gears);
			gears->UnReference();
		engine->SetMember("shiftUpRPM", params.engine.shiftUpRPM);
		engine->SetMember("shiftDownRPM", params.engine.shiftDownRPM);
		engine->SetMember("boostForce", params.engine.boostForce);
		engine->SetMember("boostDuration", params.engine.boostDuration);
		engine->SetMember("boostDelay", params.engine.boostDelay);
		engine->SetMember("boostMaxSpeed", params.engine.boostMaxSpeed);
		engine->SetMember("torqueBoost", params.engine.torqueBoost);

	table->SetMember("engine", engine);

		ILuaObject *steering = gLua->GetNewTable();
		steering->SetMember("degreesSlow", params.steering.degreesSlow);
		steering->SetMember("degreesFast", params.steering.degreesFast);
		steering->SetMember("degreesBoost", params.steering.degreesBoost);
		steering->SetMember("speedSlow", params.steering.speedSlow);
		steering->SetMember("speedFast", params.steering.speedFast);
		steering->SetMember("powerSlideAccel", params.steering.powerSlideAccel);
		steering->SetMember("steeringExponent", params.steering.steeringExponent);
		steering->SetMember("isSkidAllowed", params.steering.isSkidAllowed);
		steering->SetMember("dustCloud", params.steering.dustCloud);
		steering->SetMember("turnThrottleReduceSlow", params.steering.turnThrottleReduceSlow);
		steering->SetMember("turnThrottleReduceFast", params.steering.turnThrottleReduceFast);
		steering->SetMember("boostSteeringRateFactor", params.steering.boostSteeringRateFactor);

	table->SetMember("steering", steering);

	gLua->Push(table);

	steering->UnReference();
	engine->UnReference();
	axles->UnReference();
	body->UnReference();
	table->UnReference();

	return 1;
}

LUA_FUNCTION(GetWheelMaterial)
{
	LUA_GETVEHICLE
	gLua->CheckType(2, GLua::TYPE_NUMBER);

	IPhysicsObject *wheel = vehicle->GetWheel(gLua->GetInteger(2));

	if(!wheel)
		return 0;

	const char *mat = surfaceprop->GetPropName(wheel->GetMaterialIndex());

	gLua->Push(mat);
	return 1;
}

LUA_FUNCTION(SetWheelMaterial)
{
	LUA_GETVEHICLE
	gLua->CheckType(2, GLua::TYPE_NUMBER);
	gLua->CheckType(3, GLua::TYPE_STRING);

	IPhysicsObject *wheel = vehicle->GetWheel(gLua->GetInteger(2));

	if(!wheel)
		return 0;

	int matid = surfaceprop->GetSurfaceIndex(gLua->GetString(3));

	wheel->SetMaterialIndex(matid);

	return 0;
}

LUA_FUNCTION(GetUcmdButtons)
{
	ILuaInterface *gLua = Lua();

	gLua->CheckType(1, GLua::TYPE_USERCMD);

	CUserCmd *cmd = (CUserCmd *)gLua->GetUserData(1);

	gLua->PushLong(cmd->buttons);

	return 1;
}

LUA_FUNCTION(SetUcmdButtons)
{
	ILuaInterface *gLua = Lua();

	gLua->CheckType(1, GLua::TYPE_USERCMD);
	gLua->CheckType(2, GLua::TYPE_NUMBER);

	CUserCmd *cmd = (CUserCmd *)gLua->GetUserData(1);
	int buttons = gLua->GetInteger(2);

	cmd->buttons = buttons;

	return 0;
}

int Start(lua_State *L)
{
	CreateInterfaceFn interfaceFactory = Sys_GetFactory( ENGINE_LIB );
	CreateInterfaceFn gameServerFactory = Sys_GetFactory( SERVER_LIB );
	CreateInterfaceFn physicsFactory = Sys_GetFactory( VPHYSICS_LIB );

	surfaceprop = (IPhysicsSurfaceProps*)physicsFactory(VPHYSICS_SURFACEPROPS_INTERFACE_VERSION, NULL);

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);

	ILuaInterface *gLua = Lua();

	ILuaObject *metalist = gLua->GetGlobal("_R");
	ILuaObject *vmeta = metalist->GetMember("Vehicle");

		vmeta->SetMember("GetOperatingParams", getoperatingparams);
		vmeta->SetMember("GetVehicleParams", getvehicleparams);
		vmeta->SetMember("SetVehicleParams", setvehicleparams);
		vmeta->SetMember("SetWheelMaterial", SetWheelMaterial);
		vmeta->SetMember("GetWheelMaterial", GetWheelMaterial);

	vmeta->UnReference();
	metalist->UnReference();

	gLua->SetGlobal("GetUcmdButtons", GetUcmdButtons);
	gLua->SetGlobal("SetUcmdButtons", SetUcmdButtons);

	return 0;
}

int Close(lua_State *L)
{
	return 0;
}
