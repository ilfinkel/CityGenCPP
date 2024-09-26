// Fill out your copyright notice in the Description page of Project Settings.


#include "AllGeometry.h"

TOptional<Conn> Node::get_next_point(TSharedPtr<Point> point)
{
	for (auto c : conn)
	{
		if (c.node->get_point() == point->point)
		{
			return c;
		}
	}
	return TOptional<Conn>();
}

TOptional<Conn> Node::get_prev_point(TSharedPtr<Point> point)
{
	auto prev_point = get_next_point(point);
	if (prev_point.IsSet())
	{
		return prev_point->node->get_next_point(get_node());
	}
	return TOptional<Conn>();
}

void Node::add_conn(const TSharedPtr<Node>& node_)
{
	TSharedPtr<TArray<TSharedPtr<Point>>> final_figure;
	//looking for connection from previous rightest node to current node. 
	// We will take figure from this to our node, and then cut everything to this node out;
	TArray<TSharedPtr<Point>> added_figure;
	TSharedPtr<Node> rightest_node;
	double smallest_angle = 360;
	// trying to find rightest node and its conn before our new connection, because it contains or figure
	// so we need to change all pointers between nodes of this figure to new figure we create.
	for (int i = 0; i < conn.Num(); i++)
	{
		double angle = AllGeometry::calculate_angle(node_->get_point(), node->point, conn[i].node->get_point());
		if (angle < smallest_angle)
		{
			rightest_node = node_->conn[i].node;
			smallest_angle = angle;
		}
	}

	TArray<TSharedPtr<Point>> prev_figure;
	auto prev_point = get_prev_point(get_node());
	if (rightest_node.IsValid() && prev_point.IsSet())
	{
		TArray<TSharedPtr<Point>> fig_to_split = *prev_point->figure;
		int i = 0;
		for (; i < fig_to_split.Num(); i++)
		{
			if (fig_to_split[i]->point == get_point())
			{
				break;
			}
		}
		TArray<TSharedPtr<Point>> fig_part1;
		TArray<TSharedPtr<Point>> fig_part2;
		fig_part1.Append(&fig_to_split[0], i);
		fig_part2.Append(&fig_to_split[i], fig_to_split.Num() - i);
		auto prev_point_local = get_next_point(fig_part1[fig_part1.Num() - 2]);
		for (int fp_count = fig_part1.Num() - 1; fp_count >= 1; fp_count--)
		{
			if (!prev_point_local.IsSet())
			{
				break;
			}
			auto next_point_local = prev_point_local->node->get_next_point(fig_part2[fp_count]);
			if (!next_point_local.IsSet())
			{
				break;
			}
			next_point_local->figure = final_figure;
			if (fp_count - 2 >= 0)
			{
				prev_point_local = next_point_local->node->get_next_point(fig_part1[fp_count - 2]);
			}
		}
		auto next_point_local = get_next_point(fig_part2[0]);
		for (int fp_count = 0; fp_count < fig_part2.Num(); fp_count++)
		{
			if (!next_point_local.IsSet())
			{
				break;
			}
			next_point_local->figure = MakeShared<TArray<TSharedPtr<Point>>>(fig_part2);
			if (fp_count + 1 < fig_part2.Num())
			{
				next_point_local = next_point_local->node->get_next_point(fig_part2[fp_count + 1]);
			}
		}
		prev_figure.Insert(fig_part1, 0);
	}
	else // this is first node, so we just take this node to this figure
	{
		prev_figure.Add(MakeShared<Point>(get_point()));
	}


	//looking for connection from current node to rightest node from our next node . 
	// We will take figure from this to our node, and then cut everything from this node out;
	TArray<TSharedPtr<Point>> add_to_figure;
	TSharedPtr<Node> rightest_node2;
	smallest_angle = 360;

	for (int i = 0; i < node_->conn.Num(); i++)
	{
		double angle = AllGeometry::calculate_angle(node_->conn[i].node->get_point(), node_->get_point(), get_point());
		if (angle < smallest_angle)
		{
			rightest_node2 = node_->conn[i].node;
			smallest_angle = angle;
		}
	}
	TArray<TSharedPtr<Point>> next_figure;
	if (rightest_node2.IsValid() && node_->get_next_point(rightest_node2->node).IsSet())
	{
		auto next_point = node_->get_next_point(rightest_node2->node);
		if (next_point.IsSet())
		{
			TArray<TSharedPtr<Point>> fig_to_split = *next_point->figure;
			int i = 0;
			for (; i < fig_to_split.Num(); i++)
			{
				if (fig_to_split[i]->point == node_->get_point())
				{
					break;
				}
			}
			TArray<TSharedPtr<Point>> fig_part1;
			TArray<TSharedPtr<Point>> fig_part2;
			if (i - 1 > 0)
			{
				fig_part1.Append(&fig_to_split[0], i - 1);
				fig_part2.Append(&fig_to_split[i], fig_to_split.Num() - i + 1);
			}
			auto prev_point_local = get_next_point(fig_part1[fig_part1.Num() - 2]);
			for (int fp_count = fig_part1.Num() - 1; fp_count >= 1; fp_count--)
			{
				if (!prev_point_local.IsSet())
				{
					break;
				}
				auto next_point_local = prev_point_local->node->get_next_point(fig_part2[fp_count]);
				if (!next_point_local.IsSet())
				{
					break;
				}
				next_point_local->figure = final_figure;
				if (fp_count - 2 >= 0)
				{
					prev_point_local = next_point_local->node->get_next_point(fig_part1[fp_count - 2]);
				}
			}
			auto next_point_local = get_next_point(fig_part2[0]);
			for (int fp_count = 0; fp_count < fig_part2.Num(); fp_count++)
			{
				if (!next_point_local.IsSet())
				{
					break;
				}
				next_point_local->figure = MakeShared<TArray<TSharedPtr<Point>>>(fig_part2);
				if (fp_count + 1 < fig_part2.Num())
				{
					next_point_local = next_point_local->node->get_next_point(fig_part2[fp_count + 1]);
				}
			}
			add_to_figure.Insert(fig_part2, 0);
		}
	}
	else // this is last node, so we just take this node to this figure
	{
		add_to_figure.Add(MakeShared<Point>(node_->get_point()));
	}
	add_to_figure.Insert(prev_figure, 0);
	final_figure = MakeShared<TArray<TSharedPtr<Point>>>(add_to_figure);
}

TOptional<FVector> AllGeometry::is_intersect(const FVector& line1_begin,
                                             const FVector& line1_end,
                                             const FVector& line2_begin,
                                             const FVector& line2_end, bool is_opened)
{
	double dx1 = line1_end.X - line1_begin.X;
	double dy1 = line1_end.Y - line1_begin.Y;
	double dx2 = line2_end.X - line2_begin.X;
	double dy2 = line2_end.Y - line2_begin.Y;

	double det = dx1 * dy2 - dx2 * dy1;

	if (std::abs(det) < 1e-6)
	{
		return TOptional<FVector>();
	}

	double t1 = ((line2_begin.X - line1_begin.X) * dy2 -
			(line2_begin.Y - line1_begin.Y) * dx2) /
		det;
	double t2 = ((line2_begin.X - line1_begin.X) * dy1 -
			(line2_begin.Y - line1_begin.Y) * dx1) /
		det;

	if (t1 >= 0.0 && t1 <= 1.0 && t2 >= 0.0 && t2 <= 1.0)
	{
		FVector intersectionPoint(line1_begin.X + t1 * dx1,
		                          line1_begin.Y + t1 * dy1, 0);
		if (is_opened)
		{
			UE_LOG(LogTemp, Warning, TEXT("intersected!"));
			UE_LOG(LogTemp, Warning, TEXT("Point: %f,%f "), intersectionPoint.X,
			       intersectionPoint.Y);
			return intersectionPoint;
		}
		if (!is_opened && (FVector::Distance(intersectionPoint, line2_begin) >
			TNumericLimits<double>::Min() &&
			FVector::Distance(intersectionPoint, line2_end) >
			TNumericLimits<double>::Min() &&
			FVector::Distance(intersectionPoint, line1_begin) >
			TNumericLimits<double>::Min() &&
			FVector::Distance(intersectionPoint, line1_end) >
			TNumericLimits<double>::Min()))
		{
			return intersectionPoint;
		}
	}

	return TOptional<FVector>();
}


TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>>
AllGeometry::is_intersect_array(const TSharedPtr<Node>& line1_begin,
                                const TSharedPtr<Node>& line1_end,
                                const TArray<TSharedPtr<Node>>& lines,
                                bool is_opened)
{
	TTuple<TSharedPtr<Node>, TSharedPtr<Node>> point_line;
	double dist = TNumericLimits<double>::Max();
	FVector intersect_point_final(0, 0, 0);
	for (auto& line : lines)
	{
		for (auto& conn : line->conn)
		{
			TOptional<FVector> int_point = is_intersect(
				line1_begin->get_point(), line1_end->get_point(), line->get_point(), conn.node->get_point(), is_opened);
			if (int_point.IsSet())
			{
				double dist_to_line = FVector::Dist(line1_begin->get_point(), int_point.GetValue());
				if (dist_to_line < dist)
				{
					// point_line = PointLine(line->node, conn->node);
					point_line = TTuple<TSharedPtr<Node>, TSharedPtr<Node>>{line, conn.node};

					dist = dist_to_line;
					intersect_point_final = int_point.GetValue();
				}
			}
		}
	}
	if (dist == TNumericLimits<double>::Max())
	{
		return TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>>();
	}
	TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>> final_tuple{intersect_point_final, point_line};
	return final_tuple;
}

TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>> AllGeometry::is_intersect_array(
	FVector line1_begin, FVector line1_end, const TArray<TSharedPtr<Node>>& lines, bool is_opened)
{
	TTuple<TSharedPtr<Node>, TSharedPtr<Node>> point_line;
	double dist = TNumericLimits<double>::Max();
	FVector intersect_point_final(0, 0, 0);
	for (auto& line : lines)
	{
		for (auto& conn : line->conn)
		{
			TOptional<FVector> int_point = is_intersect(
				line1_begin, line1_end, line->get_point(), conn.node->get_point(), is_opened);
			if (int_point.IsSet())
			{
				double dist_to_line = FVector::Dist(line1_begin, int_point.GetValue());
				if (dist_to_line < dist)
				{
					// point_line = PointLine(line->node, conn->node);
					point_line = TTuple<TSharedPtr<Node>, TSharedPtr<Node>>{line, conn.node};

					dist = dist_to_line;
					intersect_point_final = int_point.GetValue();
				}
			}
		}
	}
	if (dist == TNumericLimits<double>::Max())
	{
		return TOptional<TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>>>();
	}
	TTuple<FVector, TTuple<TSharedPtr<Node>, TSharedPtr<Node>>> final_tuple{intersect_point_final, point_line};
	return final_tuple;
}


TOptional<TSharedPtr<Node>> AllGeometry::is_intersect_array_clear(const TSharedPtr<Node>& line1_begin,
                                                                  const TSharedPtr<Node>& line1_end,
                                                                  const TArray<TSharedPtr<Node>>& lines, bool is_opened)
{
	auto inter_segment = is_intersect_array(line1_begin, line1_end, lines, is_opened);
	if (!inter_segment.IsSet())
	{
		return TOptional<TSharedPtr<Node>>();
	}
	return FVector::Dist(inter_segment->Key, inter_segment->Value.Key->get_point()) < FVector::Dist(
		       inter_segment->Key, inter_segment->Value.Value->get_point())
		       ? inter_segment->Value.Key
		       : inter_segment->Value.Value;
}

TOptional<FVector> AllGeometry::is_intersect_array_clear(const FVector& line1_begin, const FVector& line1_end,
                                                         const TArray<TSharedPtr<Node>>& lines, bool is_opened)
{
	auto inter_segment = is_intersect_array(line1_begin, line1_end, lines, is_opened);
	if (!inter_segment.IsSet())
	{
		return TOptional<FVector>();
	}
	return FVector::Dist(inter_segment->Key, inter_segment->Value.Key->get_point()) < FVector::Dist(
		       inter_segment->Key, inter_segment->Value.Value->get_point())
		       ? inter_segment->Value.Key->get_point()
		       : inter_segment->Value.Value->get_point();
}

FVector AllGeometry::create_segment_at_angle(const FVector& line_begin, const FVector& line_end,
                                             const FVector& line_beginPoint, double angle_in_degrees, double length)
{
	FVector line_direction = (line_end - line_begin).GetSafeNormal();
	FVector rotated_direction = line_direction.RotateAngleAxis(angle_in_degrees, FVector(0.f, 0.f, 1.f));
	FVector line_endPoint = line_beginPoint + rotated_direction * length;

	return line_endPoint;
}

double AllGeometry::calculate_angle(const FVector& A, const FVector& B,
                                    const FVector& C)
{
	FVector BA = A - B;
	FVector BC = C - B;
	BA.Normalize();
	BC.Normalize();
	double CosTheta = FVector::DotProduct(BA, BC);
	CosTheta = FMath::Clamp(CosTheta, -1.0f, 1.0f);
	double AngleRadians = FMath::Acos(CosTheta);
	double AngleDegrees = FMath::RadiansToDegrees(AngleRadians);
	return AngleDegrees;
}
