#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <fcntl.h>
#include <set>

using namespace std;

struct result
{
    string abstractFileName;
    float score;
    string summary;
    bool isSet = false;
};

struct result *results;

set<string> queryWords;
string *abstractFiles;
int T, A, N, unprocessedAbstractCount;

pthread_mutex_t lockAbstractFile = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t printOutputLock = PTHREAD_MUTEX_INITIALIZER;

void print_abstractFiles()
{
    for (int i = 0; i < A; i++)
    {
        printf("%s\n", abstractFiles[i].c_str());
    }
}

void *abstractor(void *arg)
{
    char *threadName = (char *) arg;
    while (unprocessedAbstractCount)
    {

        struct result abstractResult;
        abstractResult.isSet = true;

        pthread_mutex_lock(&lockAbstractFile);
        if (unprocessedAbstractCount)
        {
            for (int i = 0; i < A; i++)
            {
                if (abstractFiles[i] != "")
                {
                    abstractResult.abstractFileName = abstractFiles[i];
                    printf("Thread %c is calculating %s\n", *threadName, abstractResult.abstractFileName.c_str());
                    abstractFiles[i] = "";
                    unprocessedAbstractCount--;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&lockAbstractFile);

        ifstream abstractFile(abstractResult.abstractFileName);
        string token;
        set<string> abstractWordSet;
        while (abstractFile >> token)
        {
            if (token != "." || token != "\n")
            {
                abstractWordSet.insert(token);
            }
        }

        float intersectionSetSize = 0;
        set<string>::iterator queryWordsItr, abstractWordSetItr;
        for (queryWordsItr = queryWords.begin(); queryWordsItr != queryWords.end(); queryWordsItr++)
        {
            for (abstractWordSetItr = abstractWordSet.begin(); abstractWordSetItr != abstractWordSet.end(); abstractWordSetItr++)
            {
                if (*queryWordsItr == *abstractWordSetItr)
                {
                    intersectionSetSize++;
                    break;
                }
            }
        }

        abstractResult.score = intersectionSetSize / (queryWords.size() + abstractWordSet.size() - intersectionSetSize);

        pthread_mutex_lock(&printOutputLock);

        if (abstractResult.score > results[N - 1].score)
        {
            results[N-1] = abstractResult;
            for (int i = N - 2; i >= 0; i--)
            {
                if (results[i].score < abstractResult.score)
                {
                    result temp = results[i];
                    results[i] = abstractResult;
                    results[i + 1] = temp;
                }
            }
        }

        pthread_mutex_unlock(&printOutputLock);
    }

    return 0;
}

int main(int argc, char const *argv[])
{
    const char *inputFileName = argv[1];
    ifstream inputFile(inputFileName);

    int outputFile = open(argv[2], O_WRONLY | O_CREAT, 0777);
    dup2(outputFile, STDOUT_FILENO);

    string line;
    getline(inputFile, line);
    istringstream stream1(line);
    stream1 >> T >> A >> N;

    abstractFiles = new string[A];
    unprocessedAbstractCount = A;
    results = new result[N];

    getline(inputFile, line);
    istringstream stream2(line);
    string word;
    while (stream2 >> word)
    {
        queryWords.insert(word);
    }

    int i = 0;
    while (getline(inputFile, abstractFiles[i]))
    {
        i++;
    }

    char threadNames[T];
    pthread_t threadIds[T];
    for (int i = 0; i < T; i++)
    {
        threadNames[i] = char (65 + i);
        pthread_create(&threadIds[i], NULL, &abstractor, &threadNames[i]);
    }


    for (int i = 0; i < T; i++)
    {
        pthread_join(threadIds[i], NULL);
    }
    printf("###\n"); 

    for(int i=0; i<N; i++) {
        printf("Result %d:\n", i+1);
        printf("File: %s\n", results[i].abstractFileName.c_str());
        printf("Score: %f\n", results[i].score);
        // printf("Summary: %s\n", results[i].summary);
        printf("###\n");
    }


}