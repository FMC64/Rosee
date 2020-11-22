#pragma once

#include "Brush.hpp"
#include "Static.hpp"

namespace Rosee {

class Map
{
	struct Range
	{
		size_t begin;
		size_t end;

		bool operator<(const Range &other) const
		{
			return end <= other.begin;
		}
	};

	Static::map<vector<cmp_id>, Brush> m_brushes;
	Static::map<Range, std::pair<Brush&, size_t>> m_ids;
	size_t m_id = 0;

	friend class Brush;

	void add_range(size_t begin, size_t end, Brush &b, size_t b_ndx);

public:
	Map(void);
	~Map(void);

private:
	Brush& brush_resolve(const vector<cmp_id> &key);

public:
	template <typename ...Components>
	Brush& brush(void)
	{
		constexpr auto sign = Cmp::make_id_array<Components...>();
		size_t fake_vec[3] = {sign.size(), 0, reinterpret_cast<size_t>(sign.data())};
		return brush_resolve(*reinterpret_cast<vector<cmp_id>*>(fake_vec));
	}

	template <typename ...Components>
	std::pair<Brush&, size_t> addBrush(size_t count)
	{
		auto &b = brush<Components...>();
		auto res = b.add(count);
		return std::pair<Brush&, size_t>(b, res);
	}

	template <typename ...Components>
	size_t add(size_t count)
	{
		{
			constexpr auto sign = Cmp::make_id_array<Components...>();
			static_assert(sign.size() > 0, "Id requested but components size is 0");
			static_assert(sign.cdata()[0] == Id::id, "Id not present in components list");
		}
		auto &b = brush<Components...>();
		auto b_ndx = b.add(count);
		auto res = b.template get<Id>()[b_ndx];
		return res;
	}

	std::pair<Brush*, size_t> find(size_t id);

	void remove(size_t id, size_t count);

private:
	using BrushCb = void (Brush &b, void *data);
	void query_imp(const array<cmp_id> &comps, BrushCb *cb, void *data);

public:
	template <typename ...Components, typename Callback>
	void query(Callback &&callback)
	{
		struct CallPayload {
			Callback &&cb;
		} call_payload { std::forward<Callback>(callback) };
		query_imp(Cmp::make_id_array<Components...>(), [](Brush &b, void *data){
			reinterpret_cast<CallPayload*>(data)->cb(b);
		}, &call_payload);
	}
};

}