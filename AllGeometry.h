// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */

enum point_type { main, main_road, road, river };

struct WeightedPoint
{
	WeightedPoint(const FVector& point_, const double weight_)
		: point(point_), weight(weight_)
	{
	};
	FVector point;
	double weight;
};

struct Node
{
	Node(double X, double Y, double Z) : node(FVector(X, Y, Z))
	{
	};

	Node() : node(FVector(0, 0, 0))
	{
	};

	Node(FVector node_) : Node(node_.X, node_.Y, node_.Z)
	{
	};

	FVector node;
	point_type type;
	bool used = false;
	TArray<TSharedPtr<Node>> conn;

	friend bool operator==(const Node& Lhs, const Node& RHS)
	{
		return Lhs.node == RHS.node
			&& Lhs.type == RHS.type;
	}

	friend bool operator!=(const Node& Lhs, const Node& RHS)
	{
		return !(Lhs == RHS);
	}

	void delete_me()
	{
		for (auto c : conn)
		{
			for (int i = 0; i < c->conn.Num(); i++)
			{
				if (node == c->conn[i]->node)
				{
					c->conn.RemoveAt(i);
					break;
				}
			}
		}
	};
};


static double x_size = 5000;
static double y_size = 5000;

class CITY_API AllGeometry
{
public:
	AllGeometry();
	~AllGeometry();
public:
	static TOptional<FVector> is_intersect(FVector line1_begin,
		FVector line1_end, FVector line2_begin,
		FVector line2_end, bool is_opened);


	static TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>> is_intersect_array(
		TSharedPtr<Node> line1_begin,
		TSharedPtr<Node> line1_end, const TArray<TSharedPtr<Node>> lines, bool is_opened);
	static TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>> is_intersect_array(
		FVector line1_begin,
		FVector line1_end, const TArray<TSharedPtr<Node>> lines, bool is_opened);
	static TOptional<TSharedPtr<Node>> is_intersect_array_clear(
		TSharedPtr<Node> line1_begin, TSharedPtr<Node> line1_end, const TArray<TSharedPtr<Node>> lines, bool is_opened);
	static TOptional<FVector> is_intersect_array_clear(
		FVector line1_begin, FVector line1_end, const TArray<TSharedPtr<Node>> lines, bool is_opened);
	static FVector create_segment_at_angle(const FVector& line_begin, const FVector& line_end,
		const FVector& line_beginPoint,
		double angle_in_degrees, double length);
	static double calculate_angle(FVector A, FVector B, FVector C);
};
