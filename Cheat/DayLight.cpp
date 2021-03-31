#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <io.h>
#include <cstdint>
#include <cstdlib>
#include <dwmapi.h>
#include <comdef.h> 
#include <winternl.h>

#include <dwmapi.h>
#include <comdef.h> 
#include <d3d9.h>
#include <d3dx9.h>

#include <inttypes.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dwmapi.lib")

#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_dx9.h"
#include "Imgui/imgui_impl_win32.h"

#include "vector3.h"
#include "driver.h"
#include "process.h"
#include "colors.h"
#include "overlay.h"

#include <random>
#include <vector>
#include "xor.h"

using namespace std;

int Width = GetSystemMetrics(SM_CXSCREEN);
int Height = GetSystemMetrics(SM_CYSCREEN);
const MARGINS Margin = { -1 };
DWORD ScreenCenterX;
DWORD ScreenCenterY;

RECT GameRect = { NULL };
HWND GameWnd = NULL;
void create_console();

HRESULT DirectXInit(HWND hWnd);
LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
void SetupWindow();
WPARAM MainLoop();
void CleanuoD3D();

int isTopwin();
void SetWindowToTarget();

#define TopWindowGame 11
#define TopWindowMvoe 22

IDirect3D9Ex* p_Object = NULL;
IDirect3DDevice9Ex* p_Device = NULL;
D3DPRESENT_PARAMETERS p_Params = { NULL };

#define M_Name xorstr_(L" ")
HWND MyWnd = NULL;
MSG Message = { NULL };

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::string GetNameFromId(int ID)
{
	ULONG_PTR tmp = read<ULONG_PTR>(m_base + 0x6287108);

	auto object_name_base = READ64(READ64(tmp + (int)(ID / 0x4000) * 8) + 8 * (int)(ID % 0x4000)) + 0xC;
	char pBuffer[64] = { NULL };
	readwtf(object_name_base, pBuffer, sizeof(pBuffer));
	return std::string(pBuffer);
}
#define M_PI	3.14159265358979323846264338327950288419716939937510
D3DXMATRIX Matrix(Vector3 rot, Vector3 origin = Vector3(0, 0, 0))
{
	float radPitch = (rot.x * float(M_PI) / 180.f);
	float radYaw = (rot.y * float(M_PI) / 180.f);
	float radRoll = (rot.z * float(M_PI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	D3DMATRIX matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = origin.x;
	matrix.m[3][1] = origin.y;
	matrix.m[3][2] = origin.z;
	matrix.m[3][3] = 1.f;

	return matrix;
}
Vector3 WorldToScreen(Vector3 WorldLocation, FMinimalViewInfo CameraCacheL)
{
	Vector3 Screenlocation = Vector3(0, 0, 0);

	auto POV = CameraCacheL;
	Vector3 Rotation = POV.Rotation;
	D3DMATRIX tempMatrix = Matrix(Rotation);

	Vector3 vAxisX, vAxisY, vAxisZ;

	vAxisX = Vector3(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
	vAxisY = Vector3(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
	vAxisZ = Vector3(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

	Vector3 vDelta = WorldLocation - POV.Location;
	Vector3 vTransformed = Vector3(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

	if (vTransformed.z < 1.f)
		vTransformed.z = 1.f;

	float FovAngle = POV.FOV *= 1.595744680851064f;
	float ScreenCenterX = Width / 2.0f;
	float ScreenCenterY = Height / 2.0f;

	Screenlocation.x = ScreenCenterX + vTransformed.x * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;
	Screenlocation.y = ScreenCenterY - vTransformed.y * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;

	return Screenlocation;
}
D3DMATRIX MatrixMultiplication(D3DMATRIX pM1, D3DMATRIX pM2)
{
	D3DMATRIX pOut;
	pOut._11 = pM1._11 * pM2._11 + pM1._12 * pM2._21 + pM1._13 * pM2._31 + pM1._14 * pM2._41;
	pOut._12 = pM1._11 * pM2._12 + pM1._12 * pM2._22 + pM1._13 * pM2._32 + pM1._14 * pM2._42;
	pOut._13 = pM1._11 * pM2._13 + pM1._12 * pM2._23 + pM1._13 * pM2._33 + pM1._14 * pM2._43;
	pOut._14 = pM1._11 * pM2._14 + pM1._12 * pM2._24 + pM1._13 * pM2._34 + pM1._14 * pM2._44;
	pOut._21 = pM1._21 * pM2._11 + pM1._22 * pM2._21 + pM1._23 * pM2._31 + pM1._24 * pM2._41;
	pOut._22 = pM1._21 * pM2._12 + pM1._22 * pM2._22 + pM1._23 * pM2._32 + pM1._24 * pM2._42;
	pOut._23 = pM1._21 * pM2._13 + pM1._22 * pM2._23 + pM1._23 * pM2._33 + pM1._24 * pM2._43;
	pOut._24 = pM1._21 * pM2._14 + pM1._22 * pM2._24 + pM1._23 * pM2._34 + pM1._24 * pM2._44;
	pOut._31 = pM1._31 * pM2._11 + pM1._32 * pM2._21 + pM1._33 * pM2._31 + pM1._34 * pM2._41;
	pOut._32 = pM1._31 * pM2._12 + pM1._32 * pM2._22 + pM1._33 * pM2._32 + pM1._34 * pM2._42;
	pOut._33 = pM1._31 * pM2._13 + pM1._32 * pM2._23 + pM1._33 * pM2._33 + pM1._34 * pM2._43;
	pOut._34 = pM1._31 * pM2._14 + pM1._32 * pM2._24 + pM1._33 * pM2._34 + pM1._34 * pM2._44;
	pOut._41 = pM1._41 * pM2._11 + pM1._42 * pM2._21 + pM1._43 * pM2._31 + pM1._44 * pM2._41;
	pOut._42 = pM1._41 * pM2._12 + pM1._42 * pM2._22 + pM1._43 * pM2._32 + pM1._44 * pM2._42;
	pOut._43 = pM1._41 * pM2._13 + pM1._42 * pM2._23 + pM1._43 * pM2._33 + pM1._44 * pM2._43;
	pOut._44 = pM1._41 * pM2._14 + pM1._42 * pM2._24 + pM1._43 * pM2._34 + pM1._44 * pM2._44;

	return pOut;
}
FTransform GetBoneIndex(DWORD_PTR mesh, int index)
{
	DWORD_PTR bonearray = read<DWORD_PTR>(mesh + 0x440);
	return read<FTransform>(bonearray + (index * 0x48));
}
Vector3 GetBoneWithRotation(DWORD_PTR mesh, int id)
{
	FTransform bone = GetBoneIndex(mesh, id);
	FTransform ComponentToWorld = read<FTransform>(mesh + 0x1C0);
	D3DMATRIX Matrix;
	Matrix = MatrixMultiplication(bone.ToMatrixWithScale(), ComponentToWorld.ToMatrixWithScale());
	return Vector3(Matrix._41, Matrix._42, Matrix._43);
}

D3DXVECTOR4 Rect;
char* drawBuff = (char*)malloc(1024);

typedef struct _TslEntity
{
	uint64_t pObjPointer;
	int ID;

	uint64_t PlayerController;
	uint64_t mesh;

}TslEntity;
vector<TslEntity> entityList;

uint64_t CameraManager;
Vector3 LocalPos;
uint64_t apawn;
BYTE LocalTeam;

float AimFOV = 40;
bool isaimbotting;
DWORD_PTR entityx;


class CKey {
private:
	BYTE bPrevState[0x100];
public:
	CKey() {
		memset(bPrevState, 0, sizeof(bPrevState));
	}

	BOOL IsKeyPushing(BYTE vKey) {
		return GetAsyncKeyState(vKey) & 0x8000;
	}

	BOOL IsKeyPushed(BYTE vKey) {
		BOOL bReturn = FALSE;

		if (IsKeyPushing(vKey))
			bPrevState[vKey] = TRUE;
		else
		{
			if (bPrevState[vKey] == TRUE)
				bReturn = TRUE;
			bPrevState[vKey] = FALSE;
		}

		return bReturn;
	}
};
CKey Key;
double GetCrossDistance(double x1, double y1, double x2, double y2)
{
	return sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));
}
void Aimbot2(float x, float y)
{
	//std::printf("entityx2 %p.\n", entityx);
	float ScreenCenterX = (Width / 2);
	float ScreenCenterY = (Height / 2);
	int AimSpeed = 6.0f;
	float TargetX = 0;
	float TargetY = 0;

	if (x != 0)
	{
		if (x > ScreenCenterX)
		{
			TargetX = -(ScreenCenterX - x);
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
		}

		if (x < ScreenCenterX)
		{
			TargetX = x - ScreenCenterX;
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX < 0) TargetX = 0;
		}
	}

	if (y != 0)
	{
		if (y > ScreenCenterY)
		{
			TargetY = -(ScreenCenterY - y);
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY > ScreenCenterY * 2) TargetY = 0;
		}

		if (y < ScreenCenterY)
		{
			TargetY = y - ScreenCenterY;
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY < 0) TargetY = 0;
		}
	}

	mouse_event(MOUSEEVENTF_MOVE, TargetX, TargetY, NULL, NULL);

	return;
}
bool GetClosestPlayerToCrossHair(Vector3 Pos, float& max, float aimfov, DWORD_PTR entity)
{
	if (!Key.IsKeyPushing(VK_RBUTTON) || !isaimbotting)
	{
		if (entity)
		{
			float Dist = GetCrossDistance(Pos.x, Pos.y, Width / 2, Height / 2);

			if (Dist < max)
			{
				max = Dist;
				entityx = entity;
				AimFOV = aimfov;
			}
		}
	}
	return false;
}
void AimAt(DWORD_PTR entity)
{
	DWORD_PTR closestPawn = NULL;

	float ClosestDistance = FLT_MAX;

	FCameraCacheEntry camera_cache = read<FCameraCacheEntry>(CameraManager + 0x1A60);
	uint64_t rootcomponent = read<uint64_t>(entity + 0x158);
	Vector3 relativelocation = read<Vector3>(rootcomponent + 0x164);
	Vector3 World = WorldToScreen(relativelocation, camera_cache.POV);

	if (World.y != 0 || World.y != 0)
	{
		if ((GetCrossDistance(World.x, World.y, Width / 2, Height / 2) <= AimFOV * 8) || isaimbotting) {

			Aimbot2(World.x, World.y);

		}
	}
}
void Aimbot()
{
	if (entityx != 0)
	{
		if (Key.IsKeyPushing(VK_RBUTTON))
		{
			AimAt(entityx);
		}
		else
		{
			isaimbotting = false;
		}
	}
}
void AIms(DWORD_PTR entity)
{
	float max = 100.f;

	FCameraCacheEntry camera_cache = read<FCameraCacheEntry>(CameraManager + 0x1A60);
	uint64_t rootcomponent = read<uint64_t>(entity + 0x158);
	Vector3 relativelocation = read<Vector3>(rootcomponent + 0x164);
	Vector3 World = WorldToScreen(relativelocation, camera_cache.POV);

	if (GetClosestPlayerToCrossHair(World, max, AimFOV, entity))
		entityx = entity;
}

void cache()
{
	while (true)
	{
		vector<TslEntity> tmpList;

		uint64_t Uworld = read<uint64_t>(m_base + 0x63B6528);
		//printf("Uworld ->%p\n", Uworld);
		uint64_t persistent_level = read<uint64_t>(Uworld + 0x30);
		//printf("persistent_level ->%p\n", persistent_level);
		uint64_t game_instance = read<uint64_t>(Uworld + 0x168);
		//printf("game_instance ->%p\n", game_instance);

		uint64_t localplayers_array = read<uint64_t>(game_instance + 0x38);
		//printf("localplayers_array ->%p\n", localplayers_array);
		uint64_t localplayer = read<uint64_t>(localplayers_array);
		//printf("localplayer ->%p\n", localplayer);

		uint64_t player_controller = read<uint64_t>(localplayer + 0x30);
		//printf("player_controller ->%p\n", player_controller);

		uint64_t actor_array = read<uint64_t>(persistent_level + 0x98);
		int actors_count = read<int>(persistent_level + 0xA0);
		//printf("actors_count ->%i\n", actors_count);

		CameraManager = read<uint64_t>(player_controller + 0x3C8);

		apawn = read<uint64_t>(player_controller + 0x360);

		uint64_t LocalRoot = read<std::uint64_t>(apawn + 0x158);
		LocalPos = read<Vector3>(LocalRoot + 0x164);

		uint64_t localPlayerState = read<uint64_t>(apawn + 0x350);
		uint64_t localTeamComponent = read<uint64_t>(localPlayerState + 0x4B1);

		for (unsigned long i = 0; i < actors_count; ++i)
		{
			uint64_t actor = read<uint64_t>(actor_array + i * 0x8);
			//printf("actor ->%p\n", actor);

			if (actor == 0x00)
			{
				continue;
			}

			int objectid = read<int>(actor + 0x18);
			std::string gn = GetNameFromId(objectid);
			//std::cout << gn << std::endl;

			uint64_t mesh = read<uint64_t>(actor + 0x390);

			TslEntity tslEntity{ };
			tslEntity.pObjPointer = actor;
			tslEntity.ID = objectid;

			if (gn.find("PlayerCharacter_BP_C") != std::string::npos)
			{
				if (mesh != 0x00)
				{
					tslEntity.mesh = mesh;
					tmpList.push_back(tslEntity);
				}
			}
		}

		entityList = tmpList;
		Sleep(1);
	}
}
void ESP() {
	
	FCameraCacheEntry camera_cache = read<FCameraCacheEntry>(CameraManager + 0x1A60);

	float InitFovAngle = D3DXToRadian(6.0f);

	int AimbotCircleSize = tanf(InitFovAngle) * Height * (80.0f / camera_cache.POV.FOV);
	if (camera_cache.POV.FOV < 60.0f)
		AimbotCircleSize /= 2;

	DrawCircle(Width / 2, Height / 2, AimbotCircleSize, &Col.white_, 50);

	float distance;
	int revise = 0;


	auto entityListCopy = entityList;

	if (entityListCopy.empty())
	{
		return;
	}

	for (unsigned long i = 0; i < entityListCopy.size(); ++i)
	{

		TslEntity entity = entityListCopy[i];

		if (entity.pObjPointer == apawn)
		{
			continue;
		}

		uint64_t rootcomponent = read<uint64_t>(entity.pObjPointer + 0x158);
		Vector3 relativelocation = read<Vector3>(rootcomponent + 0x164);
		Vector3 World = WorldToScreen(relativelocation, camera_cache.POV);

		uint64_t mesh = entity.mesh;


		uint64_t playerstate = read<uint64_t>(entity.pObjPointer + 0x350);
		uint64_t NamePtr = read<uint64_t>(playerstate + 0x338);
		float PlayerHealth = read<float>(entity.pObjPointer + 0x818);

		uint64_t teamcomponent = read<uint64_t>(playerstate + 0x4B1);

		if (PlayerHealth > 100.0f || PlayerHealth <= 0.01f)
		{
			continue;
		}

		char buff[64];
		if (NamePtr)
		{
			readwtf(NamePtr, buff, 64);
		}

		//printf("%S\n", buff);

		if (teamcomponent == LocalTeam)
		{
			continue;
		}

		distance = LocalPos.Distance(relativelocation) / 100.f;

		std::string gn = GetNameFromId(entity.ID);

		if (gn.find("PlayerCharacter_BP_C") != std::string::npos && mesh != 0x00) {

			AIms(entity.pObjPointer);

			sprintf(drawBuff, " [%.2d]Player H:(%.1f) %0.fm ", teamcomponent, buff, PlayerHealth, distance);

			revise = strlen(drawBuff) * 7 + 28;
			DrawFilledRect(World.x - (revise / 2) + (Rect.w / 2) - 1, World.y + Rect.z - 25, revise + 1, 3, &Col.darkgreen);
			DrawFilledRect(World.x - (revise / 2) + (Rect.w / 2) - 1, World.y + Rect.z - 28, revise + 1, 16, &Col.darkgreens);
			DrawNewText(World.x - (revise / 2) + (Rect.w / 2) + 21, World.y + Rect.z - 27, &Col.SilverWhite, drawBuff);

			if (PlayerHealth >= 100)
				PlayerHealth = 100;
			int length = (int)(PlayerHealth * revise / 100);

			if (PlayerHealth >= 80)
				DrawFilledRect(World.x - (revise / 2) + (Rect.w / 2), World.y + Rect.z - 13, length, 1, &Col.greens_);
			else if (PlayerHealth < 80 && PlayerHealth > 50)
				DrawFilledRect(World.x - (revise / 2) + (Rect.w / 2), World.y + Rect.z - 13, length, 1, &Col.red_);
			else if (PlayerHealth <= 50)
				DrawFilledRect(World.x - (revise / 2) + (Rect.w / 2), World.y + Rect.z - 13, length, 1, &Col.gray_);
		}
	}
	Aimbot();
}

bool verify_game()
{
	m_pid = PIDManager::GetAowProcId();

	if (!m_pid)
		return false;

	Controller = new DriverController(m_pid);

	usermode_pid = GetCurrentProcessId();

	if (!usermode_pid)
		return false;

	printf(("> usermode_pid: %d\n"), usermode_pid);

	m_base = GetBaseAddress();

	if (!m_base)
		return false;

	printf(("> m_pid: %d base: %llx\n"), m_pid, m_base);

	return true;
}
HRESULT DirectXInit(HWND hWnd)
{
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &p_Object)))
		exit(3);

	D3DPRESENT_PARAMETERS p_Params = { 0 };
	p_Params.Windowed = TRUE;
	p_Params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	p_Params.hDeviceWindow = hWnd;
	p_Params.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	p_Params.BackBufferFormat = D3DFMT_A8R8G8B8;
	p_Params.BackBufferWidth = Width;
	p_Params.BackBufferHeight = Height;
	p_Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	p_Params.EnableAutoDepthStencil = TRUE;
	p_Params.AutoDepthStencilFormat = D3DFMT_D16;
	p_Params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	if (FAILED(p_Object->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &p_Params, 0, &p_Device)))
	{
		p_Object->Release();
		exit(4);
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX9_Init(p_Device);

	ImGui::StyleColorsClassic();
	ImGuiStyle* style = &ImGui::GetStyle();
	style->WindowPadding = ImVec2(15, 15);
	style->WindowRounding = 5.0f;
	style->FramePadding = ImVec2(5, 5);
	style->FrameRounding = 4.0f;
	style->ItemSpacing = ImVec2(12, 8);
	style->ItemInnerSpacing = ImVec2(8, 6);
	style->IndentSpacing = 25.0f;
	style->ScrollbarSize = 15.0f;
	style->ScrollbarRounding = 9.0f;
	style->GrabMinSize = 5.0f;
	style->GrabRounding = 3.0f;

	style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
	style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
	style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
	style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
	style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);

	style->WindowTitleAlign.x = 0.50f;
	style->FrameRounding = 2.0f;

	p_Object->Release();
	return S_OK;
}
void SetupWindow()
{
	//CreateThread(0, 0, (LPTHREAD_START_ROUTINE)SetWindowToTarget, 0, 0, 0);

	WNDCLASSEX wClass =
	{
		sizeof(WNDCLASSEX),
		0,
		WinProc,
		0,
		0,
		nullptr,
		LoadIcon(nullptr, IDI_APPLICATION),
		LoadCursor(nullptr, IDC_ARROW),
		nullptr,
		nullptr,
		TEXT("Test1"),
		LoadIcon(nullptr, IDI_APPLICATION)
	};

	if (!RegisterClassEx(&wClass))
		exit(1);

	GameWnd = FindWindowW(NULL, TEXT(L"Spellbreak  "));

	printf("GameWnd Found! : %p", GameWnd);

	if (GameWnd)
	{
		GetClientRect(GameWnd, &GameRect);
		POINT xy;
		ClientToScreen(GameWnd, &xy);
		GameRect.left = xy.x;
		GameRect.top = xy.y;

		Width = GameRect.right;
		Height = GameRect.bottom;
	}
	else exit(2);

	MyWnd = CreateWindowExA(NULL, "Test1", "Test1", WS_POPUP | WS_VISIBLE, GameRect.left, GameRect.top, Width, Height, NULL, NULL, 0, NULL);
	DwmExtendFrameIntoClientArea(MyWnd, &Margin);
	SetWindowLong(MyWnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
	ShowWindow(MyWnd, SW_SHOW);
	UpdateWindow(MyWnd);

	printf("Hwnd created : %p", MyWnd);

}


void render() {

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	char fpsinfo[64];
	char lol[64];
	char ent[64];
	char ents[64];
	sprintf(fpsinfo, xorstr_("Overlay FPS: %0.f"), ImGui::GetIO().Framerate);
	DrawStrokeText(30, 44, &Col.red, fpsinfo);

	sprintf(fpsinfo, xorstr_("StrawBerry"), ImGui::GetIO().Framerate);
	DrawStrokeText(30, 66, &Col.red, fpsinfo);


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)(800), (float)(600));


	ImGui::Begin("Spellbreak");
	{	
		

		static float f = 0.0f;
		float col = 0.f;
		int show = 1;
		ImGui::SetWindowSize(ImVec2(400, 400));
		ImGui::StyleColorsDark();

		ImGui::Text(u8"Strawberry ");
		ImGui::SliderFloat("Aim FOV", &AimFOV, 100, 1.0f);
		ImGui::SliderFloat("Aim Distance", &AimFOV, 100, 1.0f);

		ImGui::Checkbox("ESP", &isaimbotting);
		ImGui::Checkbox("Aimbot", &isaimbotting);
		ImGui::Checkbox("Aim FOV", &isaimbotting);
		ImGui::Checkbox("Distance", &isaimbotting);
		ImGui::Checkbox("Skeletons", &isaimbotting);
		ImGui::Checkbox("VisCheck Aim", &isaimbotting);
		ImGui::Checkbox("Aim at downed", &isaimbotting);
		ImGui::Checkbox("Player Outlines", &isaimbotting);

		ImGui::ColorEdit3("ESP Colour", (float*)&col);

		if (ImGui::Button("Toggle Watermark")) show ^= 1;
		if (ImGui::Button("Toggle FPS")) show ^= 1;

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}


	ImVec2 v = ImGui::GetWindowSize();  // v = {32, 48} ,   is tool small
	ImGui::Text("%f %f", v.x, v.y);
	if (ImGui::GetFrameCount() < 10)
		printf("Frame %d: Size %f %f\n", ImGui::GetFrameCount(), v.x, v.y);
	ImGui::End();

	ESP();

	ImGui::EndFrame();
	p_Device->SetRenderState(D3DRS_ZENABLE, false);
	p_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	p_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
	p_Device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	if (p_Device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		p_Device->EndScene();
	}
	HRESULT result = p_Device->Present(NULL, NULL, NULL, NULL);

	if (result == D3DERR_DEVICELOST && p_Device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();
		p_Device->Reset(&p_Params);
		ImGui_ImplDX9_CreateDeviceObjects();
	}
}


WPARAM MainLoop()
{
	static RECT old_rc;
	ZeroMemory(&Message, sizeof(MSG));

	while (Message.message != WM_QUIT)
	{
		if (PeekMessage(&Message, MyWnd, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}

		HWND hwnd_active = GetForegroundWindow();
		if (GetAsyncKeyState(0x23) & 1)
			exit(8);

		if (hwnd_active == GameWnd) {
			HWND hwndtest = GetWindow(hwnd_active, GW_HWNDPREV);
			SetWindowPos(MyWnd, hwndtest, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		RECT rc;
		POINT xy;

		ZeroMemory(&rc, sizeof(RECT));
		ZeroMemory(&xy, sizeof(POINT));
		GetClientRect(GameWnd, &rc);
		ClientToScreen(GameWnd, &xy);
		rc.left = xy.x;
		rc.top = xy.y;

		ImGuiIO& io = ImGui::GetIO();
		io.ImeWindowHandle = GameWnd;
		io.DeltaTime = 1.0f / 60.0f;

		POINT p;
		GetCursorPos(&p);
		io.MousePos.x = p.x - xy.x;
		io.MousePos.y = p.y - xy.y;

		if (GetAsyncKeyState(0x1)) {
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
			io.MouseClickedPos[0].x = io.MousePos.x;
			io.MouseClickedPos[0].x = io.MousePos.y;
		}
		else
			io.MouseDown[0] = false;
		if (rc.left != old_rc.left || rc.right != old_rc.right || rc.top != old_rc.top || rc.bottom != old_rc.bottom)
		{

			old_rc = rc;

			Width = rc.right;
			Height = rc.bottom;

			p_Params.BackBufferWidth = Width;
			p_Params.BackBufferHeight = Height;
			SetWindowPos(MyWnd, (HWND)0, xy.x, xy.y, Width, Height, SWP_NOREDRAW);
			p_Device->Reset(&p_Params);
		}
		render();
	}
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanuoD3D();
	DestroyWindow(MyWnd);

	return Message.wParam;
}
LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, Message, wParam, lParam))
		return true;

	switch (Message)
	{
	case WM_DESTROY:
		CleanuoD3D();
		PostQuitMessage(0);
		exit(4);
		break;
	case WM_SIZE:
		if (p_Device != NULL && wParam != SIZE_MINIMIZED)
		{
			ImGui_ImplDX9_InvalidateDeviceObjects();
			p_Params.BackBufferWidth = LOWORD(lParam);
			p_Params.BackBufferHeight = HIWORD(lParam);
			HRESULT hr = p_Device->Reset(&p_Params);
			if (hr == D3DERR_INVALIDCALL)
				IM_ASSERT(0);
			ImGui_ImplDX9_CreateDeviceObjects();
		}
		break;
	default:
		return DefWindowProc(hWnd, Message, wParam, lParam);
		break;
	}
	return 0;
}
void CleanuoD3D()
{
	if (p_Device != NULL)
	{
		p_Device->EndScene();
		p_Device->Release();
	}
	if (p_Object != NULL)
	{
		p_Object->Release();
	}
}
int isTopwin()
{
	HWND hWnd = GetForegroundWindow();
	if (hWnd == GameWnd)
		return TopWindowGame;
	if (hWnd == MyWnd)
		return TopWindowMvoe;

	return 0;
}
void SetWindowToTarget()
{
	while (true)
	{
		GameWnd = FindWindowW(TEXT(L"UnrealWindow"), NULL);
		if (GameWnd)
		{
			ZeroMemory(&GameRect, sizeof(GameRect));
			GetWindowRect(GameWnd, &GameRect);
			Width = GameRect.right - GameRect.left;
			Height = GameRect.bottom - GameRect.top;
			DWORD dwStyle = GetWindowLong(GameWnd, GWL_STYLE);
			if (dwStyle & WS_BORDER)
			{
				GameRect.top += 32;
				Height -= 39;
			}
			ScreenCenterX = Width / 2;
			ScreenCenterY = Height / 2;
			MoveWindow(MyWnd, GameRect.left, GameRect.top, Width, Height, true);
		}
		Sleep(1);
	}
}
static const char consoleNameAlphanum[] = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
int consoleNameLength = sizeof(consoleNameAlphanum) - 1;
char genRandomConsoleName()
{
	return consoleNameAlphanum[rand() % consoleNameLength];
}
void create_console()
{
	if (!AllocConsole())
	{
		char buffer[1024] = { 0 };
		return;
	}

	auto lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	auto hConHandle = _open_osfhandle(PtrToUlong(lStdHandle), _O_TEXT);
	auto fp = _fdopen(hConHandle, xorstr_("w"));

	freopen_s(&fp, xorstr_("CONOUT$"), xorstr_("w"), stdout);

	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);
}
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	create_console();

	HWND ConsWind = GetConsoleWindow();
	srand(time(0));
	std::wstring ConsoleNameStr;

	for (unsigned int i = 0; i < 20; ++i)
	{
		ConsoleNameStr += genRandomConsoleName();
	}

	SetConsoleTitle(LPCSTR(ConsoleNameStr.c_str()));

	printf("[>] seting up window\n");

	SetupWindow();

	printf("[>] Initializing DX (overlay %d)\n", MyWnd);

	DirectXInit(MyWnd);

	printf("[>] obtaining game data\n");

	verify_game();

	printf("[>] Done\n");

	HANDLE hdl = CreateThread(nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(cache), nullptr, NULL, nullptr);

	printf("[>] Cache thread started\n");

	CloseHandle(hdl);

	printf("[>] entering main loop for rendering\n");

	while (TRUE)
	{
		MainLoop();
	}

	printf("main loop exited");

	return 0;
}