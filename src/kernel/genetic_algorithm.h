#ifndef __GENETIC_ALGORITHM_H
#define __GENETIC_ALGORITHM_H

#include <algorithm> // std::shuffle
#include <random>    // std::default_random_engine
#include <chrono>    // std::chrono::system_clock
#include <queue>     // std::priority_queue
#include <tuple>     // std::tuple
#include <set>       // std::set
#include <map>       // std::map

#include "../utility/bitmap.h"

class GeneticAlgorithm
{
public:
    class Individual
    {
    public:
        Individual() {}
        Individual(const int &rows, const int &columns,
                   const GeneticAlgorithm *fatherPtr)
            : m_rows(rows), m_columns(columns), m_fatherPtr(fatherPtr)
        {
            Random();
            m_idx.resize(m_rows * m_columns);
            for (int i = 0; i < m_idx.size(); i++)
            {
                m_idx[m_pieces[i]] = i;
            }
        }
        Individual(const int &rows, const int &columns, const std::vector<int> pieces,
                   const GeneticAlgorithm *fatherPtr)
            : m_rows(rows), m_columns(columns), m_pieces(pieces),
              m_fatherPtr(fatherPtr)
        {
            m_idx.resize(m_rows * m_columns);
            for (int i = 0; i < m_idx.size(); i++)
            {
                m_idx[m_pieces[i]] = i;
            }
        }

        void Random()
        {
            m_pieces.resize(m_rows * m_columns);
            for (int i = 0; i < m_pieces.size(); i++)
            {
                m_pieces[i] = i;
            }
            std::shuffle(m_pieces.begin(), m_pieces.end(), m_fatherPtr->m_rng);
        }

        float Fitness()
        {
            if (fitness.second)
            {
                return fitness.first;
            }
            else
            {
                assert(m_fatherPtr != nullptr);
                float ret = 1. / 1000;
                for (int i = 0; i < m_rows; i++)
                {
                    for (int j = 0; j < m_columns - 1; j++)
                    {
                        ret +=
                            m_fatherPtr->m_dissimilarity_measure[m_pieces[i * m_columns + j]][m_pieces[i * m_columns + j + 1]][3];
                    }
                }
                for (int i = 0; i < m_rows - 1; i++)
                {
                    for (int j = 0; j < m_columns; j++)
                    {
                        ret +=
                            m_fatherPtr->m_dissimilarity_measure[m_pieces[i * m_columns + j]][m_pieces[(i + 1) * m_columns + j]][1];
                    }
                }
                ret = 1000 / ret;
                fitness = std::make_pair(ret, true);
                return ret;
            }
        }

        int GetEdge(const int &piece_id, const int &orientation)
        {
            int idx = m_idx[piece_id];
            if (orientation == 0 && idx >= m_columns)
            {
                return m_pieces[idx - m_columns];
            }
            if (orientation == 1 && idx < (m_rows - 1) * m_columns)
            {
                return m_pieces[idx + m_columns];
            }
            if (orientation == 2 && idx % m_columns > 0)
            {
                return m_pieces[idx - 1];
            }
            if (orientation == 3 && idx % m_columns < m_columns - 1)
            {
                return m_pieces[idx + 1];
            }
            return -1;
        }

        bool operator<(Individual &ind)
        {
            return Fitness() < ind.Fitness();
        }

        std::pair<float, bool> fitness;
        int m_rows, m_columns;
        std::vector<int> m_pieces, m_idx;
        const GeneticAlgorithm *m_fatherPtr = nullptr;
    };

    GeneticAlgorithm(const Bitmap &image, const int &piece_size,
                     const int &generation, const int &population_size, const int &elite_size)
        : m_image(image), m_piece_size(piece_size),
          m_generation(generation), m_population_size(population_size), m_elite_size(elite_size)
    {
        m_pieces = Split(m_image, m_piece_size);
        m_columns = m_rows = 512 / m_piece_size;
        m_fittest_score = 0;
        m_termination_counter = 0;

        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        m_rng = std::default_random_engine(seed);
        m_uid = std::uniform_int_distribution<int>(0, m_columns * m_rows - 1);
    }

    void AnalyzeImage()
    {
        int n = m_pieces.size();
        m_dissimilarity_measure.resize(n);
        for (int i = 0; i < n; i++)
        {
            m_dissimilarity_measure[i].resize(n);
            for (int j = 0; j < n; j++)
            {
                m_dissimilarity_measure[i][j].resize(4);
                for (int k = 0; k < 4; k++)
                {
                    m_dissimilarity_measure[i][j][k] =
                        Dissimilarity(m_pieces[i], m_pieces[j], k);
                }
            }
        }

        m_best_match_table.resize(n);
        m_best_match_table_used.resize(n);
        for (int i = 0; i < n; i++)
        {
            m_best_match_table[i].resize(4);
            m_best_match_table_used.resize(4);
            for (int k = 0; k < 4; k++)
            {
                m_best_match_table_used[i][k] = 0;
                for (int j = 0; j < n; j++)
                {
                    if (i == j)
                    {
                        continue;
                    }
                    m_best_match_table[i][k].emplace_back(m_dissimilarity_measure[i][j][k], j);
                }
                std::sort(m_best_match_table[i][k].begin(), m_best_match_table[i][k].end());
            }
        }
    }

    void Evolution()
    {
        AnalyzeImage();

        for (int i = 0; i < m_population_size; i++)
        {
            m_population.emplace_back(m_rows, m_columns, this);
        }
        std::sort(m_population.begin(), m_population.end());

        for (int generation = 0; generation < m_generation; generation++)
        {
            std::vector<Individual> new_population = GetEliteIndividuals();
            m_fittest = new_population[0];
            std::vector<std::pair<Individual, Individual>> selected_parent =
                SelectParent(m_population_size - m_elite_size);
            for (int i = 0; i < selected_parent.size(); i++)
            {
                Individual p1 = selected_parent[i].first, p2 = selected_parent[i].second;
                Individual child = CrossOver(p1, p2);
                new_population.push_back(child);
            }

            if (m_fittest.Fitness() <= m_fittest_score)
            {
                m_termination_counter++;
            }
            else
            {
                m_fittest_score = m_fittest.Fitness();
            }

            m_population = std::move(new_population);
        }
    }

    std::vector<Individual> GetEliteIndividuals()
    {
        assert(m_elite_size <= m_population.size());
        std::vector<Individual> ret;
        for (int i = 0; i < m_elite_size; i++)
        {
            ret.push_back(m_population[m_population.size() - i - 1]);
        }
        return ret;
    }

    std::vector<std::pair<Individual, Individual>> SelectParent(int cnt)
    {
        std::vector<std::pair<Individual, Individual>> ret;
        std::vector<float> interval;
        interval.resize(m_population_size + 1);
        for (int i = 0; i < m_population_size + 1; i++)
        {
            interval[i] = i;
        }
        std::vector<float> weight;
        weight.resize(m_population_size);
        for (int i = 0; i < m_population_size; i++)
        {
            weight[i] = m_population[i].Fitness();
        }
        std::piecewise_constant_distribution<float> distribution(interval.begin(), interval.end(), weight.begin());
        for (int i = 0; i < cnt; i++)
        {
            int idx1 = distribution(m_rng);
            int idx2 = distribution(m_rng);
            ret.emplace_back(m_population[idx1], m_population[idx2]);
        }
        return ret;
    }

    struct Position
    {
        Position(const int &row, const int &column) : m_row(row), m_column(column) {}

        Position operator+(const Position &p) const
        {
            return Position(m_row + p.m_row, m_column + p.m_column);
        }

        bool operator<(const Position &p) const
        {
            return m_row < p.m_row || (m_row == p.m_row && m_column < p.m_column);
        }
        int m_row, m_column;
    };

    Position delta_position[4] =
        {Position(-1, 0), Position(1, 0), Position(0, -1), Position(0, 1)};

    Individual CrossOver(Individual parent1, Individual parent2)
    {
        std::priority_queue<std::tuple<float, int, int, int, Position>> candidate_piece;
        int min_row = 0, max_row = 0, min_column = 0, max_column = 0;
        std::map<int, Position> kernel;
        std::set<Position> used_position;

        auto IsInRange = [&](const Position &position) -> bool {
            int current_row = std::abs(std::min(min_row, position.m_row)) +
                              std::abs(std::max(max_row, position.m_row));
            int current_column = std::abs(std::min(min_column, position.m_column)) +
                                 std::abs(std::max(max_column, position.m_column));
            return (current_row <= m_rows) && (current_column <= m_columns);
        };

        auto AddCandidatePiece = [&](const int &piece_id, const Position &position, const int &orientation) {
            // Shared Edge
            {
                int piece1 = parent1.GetEdge(piece_id, orientation);
                int piece2 = parent2.GetEdge(piece_id, orientation);
                if (piece1 == piece2 && !kernel.count(piece1))
                {
                    candidate_piece.push(std::make_tuple(-100, piece_id, orientation, piece1, position));
                    return;
                }
            }

            // Best Buddy Edge
            {
                int first_buddy = m_best_match_table[piece_id][orientation][0].second;
                int second_buddy = m_best_match_table[first_buddy][orientation ^ 1][0].second;
                if (second_buddy = piece_id)
                {
                    if (parent1.GetEdge(piece_id, orientation) == first_buddy ||
                        parent2.GetEdge(piece_id, orientation) == first_buddy)
                    {
                        candidate_piece.push(std::make_tuple(-10, piece_id, orientation, first_buddy, position));
                        return;
                    }
                }
            }

            // Best Match Edge
            {
                int m = m_best_match_table[piece_id][orientation].size();
                for (int i = m_best_match_table_used[piece_id][orientation]; i < m; i++)
                {
                    int idx = m_best_match_table[piece_id][orientation][i].second;
                    float priority = m_best_match_table[piece_id][orientation][i].first;
                    m_best_match_table_used[piece_id][orientation] = i + 1;
                    if (!kernel.count(idx))
                    {
                        candidate_piece.push(std::make_tuple(priority, orientation, piece_id, idx, position));
                        return;
                    }
                }
            }
        };

        auto UpdateCandidatePiece = [&](const int &piece_id, const Position &position) {
            for (int orientation = 0; orientation < 4; orientation++)
            {
                Position new_position = position + delta_position[orientation];
                if (used_position.count(new_position) && IsInRange(new_position))
                {
                    min_row = std::min(min_row, new_position.m_row);
                    max_row = std::max(max_row, new_position.m_row);
                    min_column = std::min(min_column, new_position.m_column);
                    max_column = std::max(max_column, new_position.m_column);
                    AddCandidatePiece(piece_id, new_position, orientation);
                }
            }
        };

        auto AddPieceToKernel = [&](const int &piece_id, const Position &position) {
            kernel[piece_id] = position;
            used_position.insert(position);
            UpdateCandidatePiece(piece_id, position);
        };

        //int root_piece = parent1.m_pieces[m_uid(m_rng)];
        //AddPieceToKernel(root_piece, Position(0, 0));
        while (!candidate_piece.empty())
        {
            std::tuple<float, int, int, int, Position> top = candidate_piece.top();
            candidate_piece.pop();
            Position position = std::get<4>(top);
            if (used_position.count(position))
            {
                continue;
            }

            int piece_id = std::get<3>(top);
            int orientation = std::get<2>(top);
            int orig_piece_id = std::get<1>(top);
            if (kernel.count(piece_id))
            {
                AddCandidatePiece(orig_piece_id, position, orientation);
                continue;
            }

            AddPieceToKernel(piece_id, position);
        }

        std::vector<int> pieces;
        for (int i = 0; i < m_piece_size; i++)
        {
            Position position = kernel[i];
            int idx = (position.m_row - min_row) * m_columns + (position.m_column - min_column);
            pieces[idx] = i;
        }

        Individual ret(m_rows, m_columns, pieces, this);
        return ret;
    }

    const Bitmap &m_image;
    std::vector<Bitmap> m_pieces;
    int m_rows, m_columns, m_piece_size;

    int m_generation;
    int m_population_size;
    int m_elite_size;

    std::vector<Individual> m_population;
    std::vector<std::vector<std::vector<float>>> m_dissimilarity_measure;
    std::vector<std::vector<std::vector<std::pair<float, int>>>> m_best_match_table;
    std::vector<std::vector<int>> m_best_match_table_used;
    Individual m_fittest;
    float m_fittest_score;
    int m_termination_counter;

    std::default_random_engine m_rng;
    std::uniform_int_distribution<int> m_uid;
};

#endif // __GENETIC_ALGORITHM_H