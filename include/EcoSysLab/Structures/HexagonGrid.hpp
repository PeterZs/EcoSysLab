#pragma once

#include "ecosyslab_export.h"

namespace EcoSysLab
{
	enum class HexagonGridDirection
	{
		UpLeft,
		UpRight,
		Right,
		DownRight,
		DownLeft,
		Left
	};

	typedef int HexagonCellHandle;
	template<typename CellData>
	class HexagonCell
	{
		HexagonCellHandle m_handle = -1;
		bool m_recycled = false;

		HexagonCellHandle m_upLeft = -1;
		HexagonCellHandle m_upRight = -1;
		HexagonCellHandle m_right = -1;
		HexagonCellHandle m_downRight = -1;
		HexagonCellHandle m_downLeft = -1;
		HexagonCellHandle m_left = -1;

		glm::ivec2 m_coordinate = glm::ivec2(0);

		template<typename GD, typename CD>
		friend class HexagonGrid;

	public:
		CellData m_data;

		[[nodiscard]] glm::ivec2 GetCoordinate() const;

		explicit HexagonCell(HexagonCellHandle handle);

		[[nodiscard]] glm::ivec2 GetUpLeftCoordinate() const;

		[[nodiscard]] glm::ivec2 GetUpRightCoordinate() const;

		[[nodiscard]] glm::ivec2 GetRightCoordinate() const;

		[[nodiscard]] glm::ivec2 GetDownRightCoordinate() const;

		[[nodiscard]] glm::ivec2 GetDownLeftCoordinate() const;

		[[nodiscard]] glm::ivec2 GetLeftCoordinate() const;

		[[nodiscard]] HexagonCellHandle GetHandle() const;

		[[nodiscard]] HexagonCellHandle GetUpLeftHandle() const;

		[[nodiscard]] HexagonCellHandle GetUpRightHandle() const;

		[[nodiscard]] HexagonCellHandle GetRightHandle() const;

		[[nodiscard]] HexagonCellHandle GetDownRightHandle() const;

		[[nodiscard]] HexagonCellHandle GetDownLeftHandle() const;

		[[nodiscard]] HexagonCellHandle GetLeftHandle() const;

		[[nodiscard]] bool IsBoundary() const;
		[[nodiscard]] bool IsRecycled() const;
	};

	typedef int HexagonGridHandle;

	template<typename GridData, typename CellData>
	class HexagonGrid
	{
		std::vector<HexagonCell<CellData>> m_cells;
		std::map<std::pair<int, int>, HexagonCellHandle> m_cellMap;
		std::set<HexagonCellHandle> m_boundary;

		std::queue<HexagonCellHandle> m_cellPool;
		int m_version = -1;

		HexagonGridHandle m_handle = -1;
		bool m_recycled = false;

		template<typename GD, typename CD>
		friend class HexagonGridGroup;
	public:
		GridData m_data;
		[[nodiscard]] size_t GetCellCount() const;
		[[nodiscard]] bool IsRecycled() const;

		[[nodiscard]] HexagonGridHandle GetHandle() const;

		[[nodiscard]] glm::vec2 GetPosition(const glm::ivec2& coordinate) const;

		[[nodiscard]] glm::ivec2 GetCoordinate(const glm::vec2& position) const;

		[[nodiscard]] HexagonCellHandle Allocate(const glm::ivec2& coordinate);

		void RecycleCell(HexagonCellHandle handle);

		[[nodiscard]] const std::set<HexagonCellHandle>& GetBoundary() const;

		[[nodiscard]] glm::ivec2 FindAvailableCoordinate(HexagonCellHandle targetHandle, const glm::vec2& direction);

		[[nodiscard]] HexagonCell<CellData>& RefCell(HexagonCellHandle handle);

		[[nodiscard]] const HexagonCell<CellData>& PeekCell(HexagonCellHandle handle) const;

		[[nodiscard]] HexagonCellHandle GetCellHandle(const glm::ivec2& coordinate) const;

		[[nodiscard]] const std::vector<HexagonCell<CellData>>& PeekCells() const;

		[[nodiscard]] const std::map<std::pair<int, int>, HexagonCellHandle>& PeekCellMap() const;

		void Copy(const HexagonGrid<GridData, CellData>& src);

		explicit HexagonGrid(HexagonGridHandle handle);
	};

	

	template<typename GridData, typename CellData>
	class HexagonGridGroup
	{
		std::vector<HexagonGrid<GridData, CellData>> m_grids;
		std::queue<HexagonGridHandle> m_gridPool;

		int m_version = -1;
	public:
		[[nodiscard]] HexagonGridHandle Allocate();

		void RecycleGrid(HexagonGridHandle handle);

		[[nodiscard]] const HexagonGrid<GridData, CellData>& PeekGrid(HexagonGridHandle handle) const;

		[[nodiscard]] HexagonGrid<GridData, CellData>& RefGrid(HexagonGridHandle handle);

		[[nodiscard]] std::vector<HexagonGrid<GridData, CellData>>& RefGrids();

		[[nodiscard]] std::queue<HexagonGridHandle>& RefGridPool();
	};

	
#pragma region Implementations

	template <typename GridData, typename CellData>
	std::vector<HexagonGrid<GridData, CellData>>& HexagonGridGroup<GridData, CellData>::RefGrids()
	{
		return m_grids;
	}

	template <typename GridData, typename CellData>
	std::queue<HexagonGridHandle>& HexagonGridGroup<GridData, CellData>::RefGridPool()
	{
		return m_gridPool;
	}

	template <typename CellData>
	glm::ivec2 HexagonCell<CellData>::GetCoordinate() const
	{
		return m_coordinate;
	}

	template <typename CellData>
	HexagonCell<CellData>::HexagonCell(const HexagonCellHandle handle)
	{
		m_handle = handle;
		m_recycled = false;

		m_upLeft = m_upRight = m_right = m_downRight = m_downLeft = m_left = -1;

		m_data = {};
		m_coordinate = glm::ivec2(0);
	}

	template <typename CellData>
	glm::ivec2 HexagonCell<CellData>::GetUpLeftCoordinate() const
	{
		return { m_coordinate.x - 1, m_coordinate.y + 1 };
	}

	template <typename CellData>
	glm::ivec2 HexagonCell<CellData>::GetUpRightCoordinate() const
	{
		return { m_coordinate.x, m_coordinate.y + 1 };
	}

	

	template <typename CellData>
	glm::ivec2 HexagonCell<CellData>::GetRightCoordinate() const
	{
		return { m_coordinate.x + 1, m_coordinate.y };
	}

	template <typename CellData>
	glm::ivec2 HexagonCell<CellData>::GetDownRightCoordinate() const
	{
		return { m_coordinate.x + 1, m_coordinate.y - 1 };
	}

	template <typename CellData>
	glm::ivec2 HexagonCell<CellData>::GetDownLeftCoordinate() const
	{
		return { m_coordinate.x, m_coordinate.y - 1 };
	}

	template <typename CellData>
	glm::ivec2 HexagonCell<CellData>::GetLeftCoordinate() const
	{
		return { m_coordinate.x - 1, m_coordinate.y };
	}

	template <typename CellData>
	HexagonCellHandle HexagonCell<CellData>::GetHandle() const
	{
		return m_handle;
	}

	template <typename CellData>
	HexagonCellHandle HexagonCell<CellData>::GetUpLeftHandle() const
	{
		return m_upLeft;
	}

	template <typename CellData>
	HexagonCellHandle HexagonCell<CellData>::GetUpRightHandle() const
	{
		return m_upRight;
	}

	template <typename CellData>
	HexagonCellHandle HexagonCell<CellData>::GetRightHandle() const
	{
		return m_right;
	}

	template <typename CellData>
	HexagonCellHandle HexagonCell<CellData>::GetDownRightHandle() const
	{
		return m_downRight;
	}

	template <typename CellData>
	HexagonCellHandle HexagonCell<CellData>::GetDownLeftHandle() const
	{
		return m_downLeft;
	}

	template <typename CellData>
	HexagonCellHandle HexagonCell<CellData>::GetLeftHandle() const
	{
		return m_left;
	}

	template <typename CellData>
	bool HexagonCell<CellData>::IsBoundary() const
	{
		return m_downLeft == -1 || m_downRight == -1 || m_left == -1 || m_right == -1 || m_upLeft == -1 || m_upRight == -1;
	}

	template <typename CellData>
	bool HexagonCell<CellData>::IsRecycled() const
	{
		return m_recycled;
	}

	template <typename GridData, typename CellData>
	size_t HexagonGrid<GridData, CellData>::GetCellCount() const
	{
		return m_cellMap.size();
	}

	template <typename GridData, typename CellData>
	bool HexagonGrid<GridData, CellData>::IsRecycled() const
	{
		return m_recycled;
	}

	template <typename GridData, typename CellData>
	HexagonGridHandle HexagonGrid<GridData, CellData>::GetHandle() const
	{
		return m_handle;
	}

	template <typename GridData, typename CellData>
	glm::vec2 HexagonGrid<GridData, CellData>::GetPosition(const glm::ivec2& coordinate) const
	{
		return { coordinate.x + coordinate.y / 2.0f,
			coordinate.y * glm::cos(glm::radians(30.0f)) };
	}

	template <typename GridData, typename CellData>
	glm::ivec2 HexagonGrid<GridData, CellData>::GetCoordinate(const glm::vec2& position) const
	{
		int y = glm::round(position.y / glm::cos(glm::radians(30.0f)));
		return { glm::round((position.x - y / 2.0f)), y };
	}

	template <typename GridData, typename CellData>
	HexagonCellHandle HexagonGrid<GridData, CellData>::Allocate(const glm::ivec2& coordinate)
	{
		if (m_cellMap.find({ coordinate.x, coordinate.y }) != m_cellMap.end())
		{
			UNIENGINE_ERROR("Cell exists!");
			return -1;
		}
		HexagonCellHandle newCellHandle;
		if (m_cellPool.empty()) {
			auto newCell = m_cells.emplace_back(m_cells.size());
			newCellHandle = newCell.m_handle;
		}
		else {
			newCellHandle = m_cellPool.front();
			m_cellPool.pop();
		}
		auto& newCell = m_cells[newCellHandle];
		newCell.m_recycled = false;

		m_version++;

		newCell.m_coordinate = coordinate;
		m_cellMap[{ coordinate.x, coordinate.y }] = newCellHandle;
		const auto coordinate1 = newCell.GetLeftCoordinate();
		auto search1 = m_cellMap.find({ coordinate1.x, coordinate1.y });
		if (search1 != m_cellMap.end())
		{
			newCell.m_left = search1->second;
			auto& targetCell = m_cells[search1->second];
			targetCell.m_right = newCellHandle;
			if(!targetCell.IsBoundary())
			{
				m_boundary.erase(search1->second);
			}
		}
		const auto coordinate2 = newCell.GetUpLeftCoordinate();
		auto search2 = m_cellMap.find({ coordinate2.x, coordinate2.y });
		if (search2 != m_cellMap.end())
		{
			newCell.m_upLeft = search2->second;
			auto& targetCell = m_cells[search2->second];
			targetCell.m_downRight = newCellHandle;
			if (!targetCell.IsBoundary())
			{
				m_boundary.erase(search2->second);
			}
		}
		const auto coordinate3 = newCell.GetUpRightCoordinate();
		auto search3 = m_cellMap.find({ coordinate3.x, coordinate3.y });
		if (search3 != m_cellMap.end())
		{
			newCell.m_upRight = search3->second;
			auto& targetCell = m_cells[search3->second];
			targetCell.m_downLeft = newCellHandle;
			if (!targetCell.IsBoundary())
			{
				m_boundary.erase(search3->second);
			}
		}

		const auto coordinate4 = newCell.GetRightCoordinate();
		auto search4 = m_cellMap.find({ coordinate4.x, coordinate4.y });
		if (search4 != m_cellMap.end())
		{
			newCell.m_right = search4->second;
			auto& targetCell = m_cells[search4->second];
			targetCell.m_left = newCellHandle;
			if (!targetCell.IsBoundary())
			{
				m_boundary.erase(search4->second);
			}
		}

		const auto coordinate5 = newCell.GetDownRightCoordinate();
		auto search5 = m_cellMap.find({ coordinate5.x, coordinate5.y });
		if (search5 != m_cellMap.end())
		{
			newCell.m_downRight = search5->second;
			auto& targetCell = m_cells[search5->second];
			targetCell.m_upLeft = newCellHandle;
			if (!targetCell.IsBoundary())
			{
				m_boundary.erase(search5->second);
			}
		}

		const auto coordinate6 = newCell.GetDownLeftCoordinate();
		auto search6 = m_cellMap.find({ coordinate6.x, coordinate6.y });
		if (search6 != m_cellMap.end())
		{
			newCell.m_downLeft = search6->second;
			auto& targetCell = m_cells[search6->second];
			targetCell.m_upRight = newCellHandle;
			if (!targetCell.IsBoundary())
			{
				m_boundary.erase(search6->second);
			}
		}
		if (newCell.IsBoundary()) m_boundary.insert(newCellHandle);
		return newCellHandle;
	}

	template <typename GridData, typename CellData>
	void HexagonGrid<GridData, CellData>::RecycleCell(HexagonCellHandle handle)
	{
		auto& cell = m_cells[handle];
		assert(!cell.m_recycled);
		auto coordinate = cell.m_coordinate;
		m_cellMap.erase({ coordinate.x, coordinate.y });

		cell.m_recycled = true;

		if (cell.m_upLeft != -1) {
			auto& targetCell = m_cells[cell.m_upLeft];
			targetCell.m_downRight = -1;
			if (targetCell.IsBoundary()) m_boundary.insert(cell.m_upLeft);
		}
		if (cell.m_upRight != -1) {
			auto& targetCell = m_cells[cell.m_upRight];
			targetCell.m_downLeft = -1;
			if (targetCell.IsBoundary()) m_boundary.insert(cell.m_upRight);
		}
		if (cell.m_right != -1) {
			auto& targetCell = m_cells[cell.m_right];
			targetCell.m_left = -1;
			if (targetCell.IsBoundary()) m_boundary.insert(cell.m_right);
		}
		if (cell.m_downRight != -1) {
			auto& targetCell = m_cells[cell.m_downRight];
			targetCell.m_upLeft = -1;
			if (targetCell.IsBoundary()) m_boundary.insert(cell.m_downRight);
		}
		if (cell.m_downLeft != -1) {
			auto& targetCell = m_cells[cell.m_downLeft];
			targetCell.m_upRight = -1;
			if (targetCell.IsBoundary()) m_boundary.insert(cell.m_downLeft);
		}
		if (cell.m_left != -1) {
			auto& targetCell = m_cells[cell.m_left];
			targetCell.m_right = -1;
			if (targetCell.IsBoundary()) m_boundary.insert(cell.m_left);
		}

		cell.m_upLeft = cell.m_upRight = cell.m_right = cell.m_downRight = cell.m_downLeft = cell.m_left = -1;

		cell.m_data = {};
		cell.m_coordinate = glm::ivec2(0);

		m_cellPool.push(handle);
		m_boundary.erase(handle);
	}

	template <typename GridData, typename CellData>
	const std::set<HexagonCellHandle>& HexagonGrid<GridData, CellData>::GetBoundary() const
	{
		return m_boundary;
	}

	template <typename GridData, typename CellData>
	glm::ivec2 HexagonGrid<GridData, CellData>::FindAvailableCoordinate(HexagonCellHandle targetHandle,
		const glm::vec2& direction)
	{
		const auto& cell = m_cells[targetHandle];
		const auto angle = glm::degrees(glm::abs(glm::atan(direction.y / direction.x)));

		HexagonGridDirection hexDirection;
		if(angle > 30.0f)
		{
			if (direction.y >= 0) {
				if (direction.x >= 0) {
					hexDirection = HexagonGridDirection::UpRight;
				}
				else
				{
					hexDirection = HexagonGridDirection::UpLeft;
				}
			}else
			{
				if (direction.x >= 0) {
					hexDirection = HexagonGridDirection::DownRight;
				}
				else
				{
					hexDirection = HexagonGridDirection::DownLeft;
				}
			}
		}else
		{
			if(direction.x >= 0)
			{
				hexDirection = HexagonGridDirection::Right;
			}else
			{
				hexDirection = HexagonGridDirection::Left;
			}
		}
		bool found = false;
		auto currentHandle = targetHandle;
		glm::ivec2 coordinate = cell.m_coordinate;
		while (!found) {
			auto& currentCell = m_cells[currentHandle];
			switch (hexDirection)
			{
			case HexagonGridDirection::UpLeft:
				{
					if(currentCell.GetLeftHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetLeftCoordinate();
					}else if(currentCell.GetUpLeftHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetUpLeftCoordinate();
					}
					else if (currentCell.GetUpRightHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetUpRightCoordinate();
					}else
					{
						const auto pos1 = GetPosition(currentCell.GetLeftCoordinate());
						const auto pos2 = GetPosition(currentCell.GetUpLeftCoordinate());
						const auto pos3 = GetPosition(currentCell.GetUpRightCoordinate());
						const auto distance1 = glm::distance(pos1, glm::closestPointOnLine(pos1, glm::vec2(0), direction * 10000.0f));
						const auto distance2 = glm::distance(pos2, glm::closestPointOnLine(pos2, glm::vec2(0), direction * 10000.0f));
						const auto distance3 = glm::distance(pos3, glm::closestPointOnLine(pos3, glm::vec2(0), direction * 10000.0f));

						if(distance1 <= distance2 && distance1 <= distance3)
						{
							currentHandle = currentCell.GetLeftHandle();
						}else if (distance2 <= distance1 && distance2 <= distance3)
						{
							currentHandle = currentCell.GetUpLeftHandle();
						}else if (distance3 <= distance1 && distance3 <= distance2)
						{
							currentHandle = currentCell.GetUpRightHandle();
						}
					}
				}
				break;
			case HexagonGridDirection::UpRight:
				{
					if (currentCell.GetUpLeftHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetUpLeftCoordinate();
					}
					else if (currentCell.GetUpRightHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetUpRightCoordinate();
					}
					else if (currentCell.GetRightHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetRightCoordinate();
					}
					else
					{
						const auto pos1 = GetPosition(currentCell.GetUpLeftCoordinate());
						const auto pos2 = GetPosition(currentCell.GetUpRightCoordinate());
						const auto pos3 = GetPosition(currentCell.GetRightCoordinate());
						const auto distance1 = glm::distance(pos1, glm::closestPointOnLine(pos1, glm::vec2(0), direction * 10000.0f));
						const auto distance2 = glm::distance(pos2, glm::closestPointOnLine(pos2, glm::vec2(0), direction * 10000.0f));
						const auto distance3 = glm::distance(pos3, glm::closestPointOnLine(pos3, glm::vec2(0), direction * 10000.0f));

						if (distance1 <= distance2 && distance1 <= distance3)
						{
							currentHandle = currentCell.GetUpLeftHandle();
						}
						else if (distance2 <= distance1 && distance2 <= distance3)
						{
							currentHandle = currentCell.GetUpRightHandle();
						}
						else if (distance3 <= distance1 && distance3 <= distance2)
						{
							currentHandle = currentCell.GetRightHandle();
						}
					}
				}
				break;
			case HexagonGridDirection::Right:
				{
					if (currentCell.GetUpRightHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetUpRightCoordinate();
					}
					else if (currentCell.GetRightHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetRightCoordinate();
					}
					else if (currentCell.GetDownRightHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetDownRightCoordinate();
					}
					else
					{
						const auto pos1 = GetPosition(currentCell.GetUpRightCoordinate());
						const auto pos2 = GetPosition(currentCell.GetRightCoordinate());
						const auto pos3 = GetPosition(currentCell.GetDownRightCoordinate());
						const auto distance1 = glm::distance(pos1, glm::closestPointOnLine(pos1, glm::vec2(0), direction * 10000.0f));
						const auto distance2 = glm::distance(pos2, glm::closestPointOnLine(pos2, glm::vec2(0), direction * 10000.0f));
						const auto distance3 = glm::distance(pos3, glm::closestPointOnLine(pos3, glm::vec2(0), direction * 10000.0f));
						if (distance1 <= distance2 && distance1 <= distance3)
						{
							currentHandle = currentCell.GetUpRightHandle();
						}
						else if (distance2 <= distance1 && distance2 <= distance3)
						{
							currentHandle = currentCell.GetRightHandle();
						}
						else if (distance3 <= distance1 && distance3 <= distance2)
						{
							currentHandle = currentCell.GetDownRightHandle();
						}
					}
				}
				break;
			case HexagonGridDirection::DownRight:
				{
					if (currentCell.GetRightHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetRightCoordinate();
					}
					else if (currentCell.GetDownRightHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetDownRightCoordinate();
					}
					else if (currentCell.GetDownLeftHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetDownLeftCoordinate();
					}
					else
					{
						const auto pos1 = GetPosition(currentCell.GetRightCoordinate());
						const auto pos2 = GetPosition(currentCell.GetDownRightCoordinate());
						const auto pos3 = GetPosition(currentCell.GetDownLeftCoordinate());
						const auto distance1 = glm::distance(pos1, glm::closestPointOnLine(pos1, glm::vec2(0), direction * 10000.0f));
						const auto distance2 = glm::distance(pos2, glm::closestPointOnLine(pos2, glm::vec2(0), direction * 10000.0f));
						const auto distance3 = glm::distance(pos3, glm::closestPointOnLine(pos3, glm::vec2(0), direction * 10000.0f));
						if (distance1 <= distance2 && distance1 <= distance3)
						{
							currentHandle = currentCell.GetRightHandle();
						}
						else if (distance2 <= distance1 && distance2 <= distance3)
						{
							currentHandle = currentCell.GetDownRightHandle();
						}
						else if (distance3 <= distance1 && distance3 <= distance2)
						{
							currentHandle = currentCell.GetDownLeftHandle();
						}
					}
				}
				break;
			case HexagonGridDirection::DownLeft:
				{
					if (currentCell.GetDownRightHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetDownRightCoordinate();
					}
					else if (currentCell.GetDownLeftHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetDownLeftCoordinate();
					}
					else if (currentCell.GetLeftHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetLeftCoordinate();
					}
					else
					{
						const auto pos1 = GetPosition(currentCell.GetDownRightCoordinate());
						const auto pos2 = GetPosition(currentCell.GetDownLeftCoordinate());
						const auto pos3 = GetPosition(currentCell.GetLeftCoordinate());
						const auto distance1 = glm::distance(pos1, glm::closestPointOnLine(pos1, glm::vec2(0), direction * 10000.0f));
						const auto distance2 = glm::distance(pos2, glm::closestPointOnLine(pos2, glm::vec2(0), direction * 10000.0f));
						const auto distance3 = glm::distance(pos3, glm::closestPointOnLine(pos3, glm::vec2(0), direction * 10000.0f));
						if (distance1 <= distance2 && distance1 <= distance3)
						{
							currentHandle = currentCell.GetDownRightHandle();
						}
						else if (distance2 <= distance1 && distance2 <= distance3)
						{
							currentHandle = currentCell.GetDownLeftHandle();
						}
						else if (distance3 <= distance1 && distance3 <= distance2)
						{
							currentHandle = currentCell.GetLeftHandle();
						}
					}
				}
				break;
			case HexagonGridDirection::Left:
				{
					if (currentCell.GetDownLeftHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetDownLeftCoordinate();
					}
					else if (currentCell.GetLeftHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetLeftCoordinate();
					}
					else if (currentCell.GetUpLeftHandle() == -1)
					{
						found = true;
						coordinate = currentCell.GetUpLeftCoordinate();
					}
					else
					{
						const auto pos1 = GetPosition(currentCell.GetDownLeftCoordinate());
						const auto pos2 = GetPosition(currentCell.GetLeftCoordinate());
						const auto pos3 = GetPosition(currentCell.GetUpLeftCoordinate());
						const auto distance1 = glm::distance(pos1, glm::closestPointOnLine(pos1, glm::vec2(0), direction * 10000.0f));
						const auto distance2 = glm::distance(pos2, glm::closestPointOnLine(pos2, glm::vec2(0), direction * 10000.0f));
						const auto distance3 = glm::distance(pos3, glm::closestPointOnLine(pos3, glm::vec2(0), direction * 10000.0f));
						if (distance1 <= distance2 && distance1 <= distance3)
						{
							currentHandle = currentCell.GetDownLeftHandle();
						}
						else if (distance2 <= distance1 && distance2 <= distance3)
						{
							currentHandle = currentCell.GetLeftHandle();
						}
						else if (distance3 <= distance1 && distance3 <= distance2)
						{
							currentHandle = currentCell.GetUpLeftHandle();
						}
					}
				}
				break;
			}
		}
		return coordinate;
	}

	template <typename GridData, typename CellData>
	HexagonCell<CellData>& HexagonGrid<GridData, CellData>::RefCell(HexagonCellHandle handle)
	{
		return m_cells[handle];
	}

	template <typename GridData, typename CellData>
	const HexagonCell<CellData>& HexagonGrid<GridData, CellData>::PeekCell(HexagonCellHandle handle) const
	{
		return m_cells[handle];
	}

	template <typename GridData, typename CellData>
	HexagonCellHandle HexagonGrid<GridData, CellData>::GetCellHandle(const glm::ivec2& coordinate) const
	{
		const auto search = m_cellMap.find({ coordinate.x, coordinate.y });
		if (search != m_cellMap.end()) return search->second;
		return -1;
	}

	template <typename GridData, typename CellData>
	const std::vector<HexagonCell<CellData>>& HexagonGrid<GridData, CellData>::PeekCells() const
	{
		return m_cells;
	}

	template <typename GridData, typename CellData>
	const std::map<std::pair<int, int>, HexagonCellHandle>& HexagonGrid<GridData, CellData>::PeekCellMap() const
	{
		return m_cellMap;
	}

	template <typename GridData, typename CellData>
	void HexagonGrid<GridData, CellData>::Copy(const HexagonGrid<GridData, CellData>& src)
	{
		m_cells = src.m_cells;
		m_cellMap = src.m_cellMap;
		m_cellPool = src.m_cellPool;
	}

	template <typename GridData, typename CellData>
	HexagonGrid<GridData, CellData>::HexagonGrid(const HexagonGridHandle handle)
	{
		m_handle = handle;
		m_recycled = false;
		m_version = -1;
	}

	template <typename GridData, typename CellData>
	HexagonGridHandle HexagonGridGroup<GridData, CellData>::Allocate()
	{
		HexagonGridHandle newGridHandle;
		if (m_gridPool.empty()) {
			auto newGrid = m_grids.emplace_back(m_grids.size());
			newGridHandle = newGrid.m_handle;
		}
		else {
			newGridHandle = m_gridPool.front();
			m_gridPool.pop();
		}
		m_version++;
		m_grids[newGridHandle].m_recycled = false;
		m_grids[newGridHandle].m_data = {};
		return newGridHandle;
	}

	template <typename GridData, typename CellData>
	void HexagonGridGroup<GridData, CellData>::RecycleGrid(HexagonGridHandle handle)
	{
		auto& grid = m_grids[handle];
		assert(!grid.m_recycled);
		grid.m_recycled = true;
		grid.m_cells.clear();
		grid.m_cellMap.clear();
		grid.m_cellPool = {};

		m_gridPool.push(handle);
	}

	template <typename GridData, typename CellData>
	const HexagonGrid<GridData, CellData>& HexagonGridGroup<GridData, CellData>::PeekGrid(HexagonGridHandle handle) const
	{
		return m_grids[handle];
	}

	template <typename GridData, typename CellData>
	HexagonGrid<GridData, CellData>& HexagonGridGroup<GridData, CellData>::RefGrid(HexagonGridHandle handle)
	{
		return m_grids[handle];
	}
#pragma endregion
}
