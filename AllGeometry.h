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
enum block_type
{
	royal,
	dock,
	luxury,
	residential,
	slums,
	empty,
	unknown
};

struct WeightedPoint
{
	WeightedPoint(const FVector& point_, const double weight_) : point(point_), weight(weight_) {};
	FVector point;
	double weight;
};

struct Point
{
	Point(double X, double Y, double Z) : point(FVector(X, Y, Z)) {};

	Point() : point(FVector(0, 0, 0)) {};

	Point(FVector node_) : Point(node_.X, node_.Y, node_.Z) {};
	~Point() { blocks_nearby.Empty(); }
	FVector point;
	point_type type;
	bool used = false;
	TArray<block_type> blocks_nearby;

	Point& operator=(const Point& Other)
	{
		if (this != &Other)
		{
			point = Other.point;
			type = Other.type;
			used = Other.used;
			blocks_nearby = Other.blocks_nearby;
		}
		return *this;
	}
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
	~Conn()
	{
		figure->Empty();
		node.Reset();
	}
	// Conn(TSharedPtr<Node> node_) :node( node_ ) {}
	TSharedPtr<Node> node;
	TSharedPtr<TArray<TSharedPtr<Point>>> figure;
	bool not_in_figure;
	bool operator==(Conn& other) { return this->node == other.node; }
};

struct Node
{
	Node(double X, double Y, double Z) : node(MakeShared<Point>(FVector(X, Y, Z))) {};

	Node() : node(MakeShared<Point>(FVector(0, 0, 0))) {};

	Node(FVector node_) : node(MakeShared<Point>(node_.X, node_.Y, node_.Z)) {};
	~Node() { conn.Empty(); }
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
	void delete_me();
	bool operator==(const Node&) const { return this->node->point == node->point; }

protected:
	TSharedPtr<Point> node;
};
struct House
{
	House(TArray<FVector> figure_, double height_) : house_figure(figure_), height(height_) {};
	~House() { house_figure.Empty(); }
	TArray<FVector> house_figure;
	double height;
};
struct Block
{
	Block()
	{
		type = block_type::unknown;
		area = 0;
		figure = TArray<TSharedPtr<Point>>();
	};
	~Block()
	{
		figure.Empty();
		self_figure.Empty();
	};
	Block(TArray<TSharedPtr<Point>> figure_);
	TArray<TSharedPtr<Point>> figure;
	TArray<Point> self_figure;
	TArray<House> houses;
	double area;
	int main_roads;
	bool is_river_in;
	void set_type(block_type type_);
	block_type get_type() { return type; };
	bool is_point_in_self_figure(FVector point_);
	bool is_point_in_figure(FVector point_);
	void get_self_figure();
	bool shrink_size(TArray<Point>& Vertices, float size_delta);
	TOptional<FVector> is_line_intersect(FVector point1, FVector point2);

	bool create_house(TArray<FVector> given_line, double width, double height);

private:
	block_type type;
};

static double x_size = 4000;
static double y_size = 4000;

class CITY_API AllGeometry
{
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
	static int is_intersect_array_count(const TSharedPtr<Node>& line_begin, const TSharedPtr<Node>& line_end,
										const TArray<TSharedPtr<Node>>& lines, bool is_opened);
	static TOptional<FVector> is_intersect_array_clear(const FVector& line_begin, const FVector& line_end,
													   const TArray<TSharedPtr<Node>>& lines, bool is_opened);
	static FVector create_segment_at_angle(const FVector& line_begin, const FVector& line_end,
										   const FVector& line_beginPoint, double angle_in_degrees, double length);
	static double calculate_angle(const FVector& A, const FVector& B, const FVector& C, bool is_clockwork = false);
	static float get_poygon_area(const TArray<TSharedPtr<Point>>& Vertices);
	static float get_poygon_area(const TArray<Point>& Vertices);
	static bool IsConvex(const FVector& Prev, const FVector& Curr, const FVector& Next);
	static bool IsPointInTriangle(const FVector& P, const FVector& A, const FVector& B, const FVector& C);
	static bool IsEar(const TArray<FVector>& Vertices, int32 PrevIndex, int32 CurrIndex, int32 NextIndex,
					  const TArray<int32>& RemainingVertices);
	static bool IsPointInsidePolygon(const FVector& Point, const TArray<FVector>& Polygon);
	static void TriangulatePolygon(const TArray<FVector>& Polygon, TArray<int32>& Triangles);
	static bool is_point_in_figure(FVector& point_, TArray<FVector>& figure);
	static float point_to_seg_distance(const FVector& SegmentStart, const FVector& SegmentEnd, const FVector& Point);
};
