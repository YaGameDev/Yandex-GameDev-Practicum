#pragma once

#include <CrySystem/ICryPlugin.h>

#include <CryGame/IGameFramework.h>

#include <CryEntitySystem/IEntityClass.h>

class CPlugin 
	: public Cry::IEnginePlugin
	, public ISystemEventListener
	//, public INetworkedClientListener
{
public:
	CRYINTERFACE_SIMPLE(Cry::IEnginePlugin)
	CRYGENERATE_SINGLETONCLASS_GUID(CPlugin, "MyPlugin", "{DF7C45D5-D2F6-494D-BD5C-B41455AB5824}"_cry_guid)

	virtual ~CPlugin();
	
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

	virtual void MainUpdate(float frameRate) override;

	//virtual void OnLocalClientDisconnected(EDisconnectionCause cause, const char* description) override {}
	//virtual bool OnClientConnectionReceived(int channelId, bool bIsReset) override { return true; };
	//virtual bool OnClientReadyForGameplay(int channelId, bool bIsReset) override { return true; };
	//virtual void OnClientDisconnected(int channelId, EDisconnectionCause cause, const char* description, bool bKeepClient) override { };
	//virtual bool OnClientTimingOut(int channelId, EDisconnectionCause cause, const char* description) override { return true; }
};