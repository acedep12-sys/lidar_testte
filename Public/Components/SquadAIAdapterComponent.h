#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/SquadAICommandTypes.h"
#include "SquadAIAdapterComponent.generated.h"

class UAimComponent;
class UWeaponComponent;
class ASoldierCharacter;

UCLASS(ClassGroup=(SquadAI), meta=(BlueprintSpawnableComponent, DisplayName = "SquadAI Adapter"))
class SQUADAI_API USquadAIAdapterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USquadAIAdapterComponent();

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Adapter")
	void InitializeForOwner();

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Adapter")
	void SetMoveCommand(const FSquadAIMoveCommand& NewCommand);

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Adapter")
	void ClearMoveCommand();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SquadAI|Adapter")
	FSquadAIMoveCommand GetMoveCommand() const { return MoveCommand; }

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Adapter")
	void SetAimCommand(const FSquadAIAimCommand& NewCommand);

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Adapter")
	void ClearAimCommand();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SquadAI|Adapter")
	FSquadAIAimCommand GetAimCommand() const { return AimCommand; }

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Adapter")
	bool TryStartBurstAtAimCommand();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SquadAI|Adapter")
	bool IsBurstFiring() const;

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Adapter")
	void StopBurstAndClearAim();

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Adapter")
	void EnablePackSafeMode(bool bEnabled);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SquadAI|Adapter")
	bool IsUsingPackSafeMode() const { return bPackSafeMode; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SquadAI|Adapter")
	bool HasAimCommand() const { return AimCommand.IsValid(); }

	UPROPERTY(BlueprintReadOnly, Category = "SquadAI|Adapter")
	TObjectPtr<ASoldierCharacter> SoldierOwner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "SquadAI|Adapter")
	TObjectPtr<UAimComponent> AimComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "SquadAI|Adapter")
	TObjectPtr<UWeaponComponent> WeaponComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SquadAI|Adapter|Pack")
	bool bPackSafeMode = false;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	FSquadAIMoveCommand MoveCommand;

	UPROPERTY()
	FSquadAIAimCommand AimCommand;

	UPROPERTY()
	TObjectPtr<UObject> LastPackAimTargetObject = nullptr;

	UPROPERTY()
	FVector LastPackAimLocation = FVector::ZeroVector;

	UPROPERTY()
	bool bPackAimingActive = false;

	UPROPERTY()
	bool bPackFiringActive = false;
};
