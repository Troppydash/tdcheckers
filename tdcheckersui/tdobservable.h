#pragma once

#include <unordered_map>
#include <functional>

namespace td
{
	template <typename T>
	class observable
	{
	public:
		using notifier = std::function<void(const T&, const T&)>;

		observable() {};
		observable(T initial) : m_item(initial) {};

		void update(T value)
		{

			for (const std::pair<int, notifier> &item : m_observers)
			{
				item.second(m_item, value);
			}

			m_item = value;
		}

		int observe(notifier noti)
		{
			m_observers[m_nextid++] = noti;
			return m_nextid - 1;
		}

		void unobserve(int id)
		{
			m_observers.erase(id);
		}

		T get()
		{
			return m_item;
		}

	protected:
		T m_item;
		std::unordered_map<int, notifier> m_observers;
		int m_nextid = 0;
	};


}