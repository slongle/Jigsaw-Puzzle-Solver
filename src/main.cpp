#include <iostream>

#include "kernel/genetic_algorithm.h"

int piece_size, generation, population_size, elite_size;

void Solve(const std::string &path, const std::string &filename)
{
    Bitmap image(path + filename + ".png");
    GeneticAlgorithm GA(image, piece_size, generation, population_size, elite_size);
    GA.Evolution();

    GeneticAlgorithm::Individual individual = GA.m_fittest;
    Bitmap answer(512, 512, 3);
    int row, column, idx = 0;
    row = column = 512 / piece_size;
    for (int i = 0; i < row; i++)
    {
        for (int j = 0; j < column; j++)
        {
            answer.Merge(GA.m_pieces[individual.m_pieces[idx]], row * piece_size, column * piece_size);
            idx++;
        }
    }
    answer.OutputPNG(filename + "_ans.png");
}

int main()
{
    piece_size = 64;
    generation = 20;
    population_size = 100;
    elite_size = 4;
    std::string path = "E:/Document/Code/HC/Round2/src/";
    std::string filename = "1200";
    Solve(path, filename);
    return 0;
}