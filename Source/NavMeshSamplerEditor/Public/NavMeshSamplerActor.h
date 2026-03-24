// NavMeshSamplerActor.h
// Actor for visualizing sampled points in the editor

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavMeshSamplerTypes.h"
#include "NavMeshSamplerActor.generated.h"

UCLASS()
class NAVMESHSAMPLEREDITOR_API ANavMeshSamplerActor : public AActor
{
	GENERATED_BODY()

public:
	ANavMeshSamplerActor(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	/** Set the points to visualize */
	UFUNCTION(BlueprintCallable, Category = "NavMesh Sampler")
	void SetSamplePoints(const TArray<FVector>& Points);

	/** Set points from sampling result */
	UFUNCTION(BlueprintCallable, Category = "NavMesh Sampler")
	void SetSamplingResult(const FNavMeshSamplingResult& Result);

	/** Clear all visualization points */
	UFUNCTION(BlueprintCallable, Category = "NavMesh Sampler")
	void ClearPoints();

	/** Get current sampling result */
	const FNavMeshSamplingResult& GetSamplingResult() const { return SamplingResult; }

protected:
	/** Draw debug visualization */
	void DrawVisualization();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NavMesh Sampler")
	FNavMeshSamplingResult SamplingResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	FColor PointColor = FColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	float PointSize = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	bool bDrawPoints = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	bool bDrawBounds = true;
};