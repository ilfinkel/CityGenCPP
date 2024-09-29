// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct Node;


enum point_type
{
	main,
	main_road,
	road,
	river
};

struct WeightedPoint
{
	WeightedPoint(const FVector& point_, const double weight_) : point(point_), weight(weight_){};
	FVector point;
	double weight;
};

struct Point
{
	Point(double X, double Y, double Z) : point(FVector(X, Y, Z)){};

	Point() : point(FVector(0, 0, 0)){};

	Point(FVector node_) : Point(node_.X, node_.Y, node_.Z){};

	FVector point;
	point_type type;
	bool used = false;


	// friend bool operator==(const Point& Lhs, const Point& RHS)
	// {
	// 	return Lhs.point == RHS.point
	// 		&& Lhs.type == RHS.type;
	// }
	//
	// friend bool operator!=(const Point& Lhs, const Point& RHS)
	// {
	// 	return !(Lhs == RHS);
	// }
};

struct Conn
{
	Conn(TSharedPtr<Node> node_, TSharedPtr<TArray<TSharedPtr<Point>>> figure_) : node(node_), figure(figure_)
	{
		not_in_figure = false;
	}

	Conn(TSharedPtr<Node> node_) : node(node_)
	{
		figure = MakeShared<TArray<TSharedPtr<Point>>>();
		not_in_figure = false;
	}

	// Conn(TSharedPtr<Node> node_) :node( node_ ) {}
	TSharedPtr<Node> node;
	TSharedPtr<TArray<TSharedPtr<Point>>> figure;
	bool not_in_figure;
};

struct Node
{
	Node(double X, double Y, double Z) : node(MakeShared<Point>(FVector(X, Y, Z))){};

	Node() : node(MakeShared<Point>(FVector(0, 0, 0))){};

	Node(FVector node_) : node(MakeShared<Point>(node_.X, node_.Y, node_.Z)){};

	TArray<TSharedPtr<Conn>> conn;
	void set_point(FVector point_) { node->point = point_; }
	void set_point_X(double X) { node->point.X = X; }
	void set_point_Y(double Y) { node->point.Y = Y; }
	void set_point_Z(double Z) { node->point.Z = Z; }
	FVector get_point() { return node->point; }
	TSharedPtr<Point> get_node() { return node; }
	bool is_used() { return node->used; }
	void set_used() { node->used = true; }
	void set_used(bool used_) { node->used = used_; }
	point_type get_type() { return node->type; }
	void set_type(point_type type_) { node->type = type_; }
	TOptional<TSharedPtr<Conn>> get_next_point(TSharedPtr<Point> point);
	TOptional<TSharedPtr<Conn>> get_prev_point(TSharedPtr<Point> point);
	void add_connection(const TSharedPtr<Node>& node_);
	void delete_me()
	{
		for (auto c : conn)
		{
			for (int i = 0; i < c->node->conn.Num(); i++)
			{
				if (node->point == c->node->conn[i]->node->get_point())
				{
					c->node->conn.RemoveAt(i);
					break;
				}
			}
		}
	};

protected:
	TSharedPtr<Point> node;
};


static double x_size = 5000;
static double y_size = 5000;

class CITY_API AllGeometry
{
public:
	AllGeometry();
	~AllGeometry();

public:
	static TOptional<FVector> is_intersect(const FVector& line1_begin, const FVector& line1_end,
										   const FVector& line2_begin, const FVector& line2_end, bool is_opened);


	static TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>> is_intersect_array(
		const TSharedPtr<Node>& line1_begin, const TSharedPtr<Node>& line1_end, const TArray<TSharedPtr<Node>>& lines,
		bool is_opened);
	static TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>> is_intersect_array(
		FVector line1_begin, FVector line1_end, const TArray<TSharedPtr<Node>>& lines, bool is_opened);
	static TOptional<TSharedPtr<Node>> is_intersect_array_clear(const TSharedPtr<Node>& line1_begin,
																const TSharedPtr<Node>& line1_end,
																const TArray<TSharedPtr<Node>>& lines, bool is_opened);
	static TOptional<FVector> is_intersect_array_clear(const FVector& line1_begin, const FVector& line1_end,
													   const TArray<TSharedPtr<Node>>& lines, bool is_opened);
	static FVector create_segment_at_angle(const FVector& line_begin, const FVector& line_end,
										   const FVector& line_beginPoint, double angle_in_degrees, double length);
	static int calculate_angle(const FVector& A, const FVector& B, const FVector& C, bool is_clockwork = false);
};
