#include <algorithm>
#include <cassert>
#include <deque>
#include <map>
#include <queue>
#include <unordered_set>
#include <set>
#include <stdexcept>
#include <iostream>

#include "Timer.h"
#include "Tokenizer.h"

using namespace std;

#define TIMER Timer timer([n = __func__, &fr = m_functionRuntimes](std::int64_t duration){ auto & v = fr[n]; v.first++; v.second += duration; })

Tokenizer::Tokenizer()
    : m_tokenizedInputSequence()
    , m_nextUnusedToken(512)
    , m_tokenPairCountAndPositions()
    , m_tokenPairReplacements()
    , m_tokenReplacements()
    , m_tokenRelationships()
    , m_functionRuntimes()
{}

Tokenizer::Tokenizer(SQLite::DB & db)
    : Tokenizer()
{
    Timer timer([n = __func__, &fr = m_functionRuntimes](std::int64_t duration)
    { auto & v = fr[n]; v.first++; v.second += duration; });
    LoadDataFromDB(db);
}

void Tokenizer::Process(const std::string& input)
{
    auto firstTokenToUpdate = m_tokenizedInputSequence.Last();
    FeedInput(input);
    UpdateTokenPairCountAndPositions(firstTokenToUpdate);
    ApplyExistingTokenPairReplacements();
    OptimizeTokenLength();
    CalculateTokenRelationships();
}

void Tokenizer::FeedInput(const std::string& input)
{
    TIMER;
    bool separated = !m_tokenizedInputSequence.Empty();
    for(uint8_t c: input)
    {
        if(isspace(c))
        {
            separated = true;
        }
        else
        {
            m_tokenizedInputSequence.EmplaceBack(c + (separated?256:0));
            separated = false;
        }
    }
}

void Tokenizer::UpdateTokenPairCountAndPositions(LinkedList<Token>::Iterator position)
{
    TIMER;
    if(position == m_tokenizedInputSequence.end())
    {
        position = m_tokenizedInputSequence.begin();
    }
    if(position == m_tokenizedInputSequence.end())
    {
        return;
    }
    auto prevPosition = position;
    for(++position; position != m_tokenizedInputSequence.end(); ++position)
    {
        const Token & left = *prevPosition;
        const Token & right = *position;
        TokenPair tokenPair(left, right);
        auto & countAndPosition = m_tokenPairCountAndPositions[tokenPair];
        countAndPosition.count++;
        countAndPosition.positions.emplace(prevPosition);
        prevPosition = position;
    }
}

void Tokenizer::ApplyExistingTokenPairReplacements()
{
    TIMER;
    auto position = m_tokenizedInputSequence.begin();
    if(position == m_tokenizedInputSequence.end() || m_tokenPairReplacements.empty())
    {
        return;
    }
    auto prevPosition = position;
    for(++position; position != m_tokenizedInputSequence.end();)
    {
        const Token & left = *prevPosition;
        const Token & right = *position;
        TokenPair tokenPair(left, right);
        auto replacementIt = m_tokenPairReplacements.find(tokenPair);
        if(replacementIt != m_tokenPairReplacements.end())
        {
            const Token & replacement = replacementIt->second;
            position = prevPosition = m_tokenizedInputSequence.ReplaceTwo(prevPosition, replacement);
            auto countAndPositionIter = m_tokenPairCountAndPositions.find(tokenPair);
            if(countAndPositionIter != m_tokenPairCountAndPositions.end())
            {
                countAndPositionIter->second.count--;
                if(countAndPositionIter->second.count <= 0)
                {
                    m_tokenPairCountAndPositions.erase(countAndPositionIter);
                }
                else
                {
                    auto & positions = countAndPositionIter->second.positions;
                    auto posIt = std::find(positions.begin(), positions.end(), position);
                    if(posIt != positions.end())
                    {
                        positions.erase(posIt); 
                    }
                }
            }
        }
        else
        {
            prevPosition = position;
        }
        ++position;
    }
}

void Tokenizer::OptimizeTokenLength()
{
    TIMER;
    if(m_tokenPairCountAndPositions.empty())
    {
        return;
    }
    struct Candidate
    {
        size_t count;
        uint64_t version;
        TokenPair pair;
    };
    struct CandidateLess
    {
        bool operator()(const Candidate& a, const Candidate& b) const
        {
            return a.count < b.count;
        }
    };
    std::priority_queue<Candidate, std::vector<Candidate>, CandidateLess> candidates;
    std::unordered_map<TokenPair, uint64_t, TokenPair::HashFunction> versions;
    auto QueueCandidate = [&](const TokenPair& pair, const CountAndPosition& data)
    {
        const auto version = ++versions[pair];
        if(!data.positions.empty())
        {
            candidates.push({data.count, version, pair});
        }
    };
    for(const auto& [pair, data] : m_tokenPairCountAndPositions)
    {
        if(!data.positions.empty())
        {
            QueueCandidate(pair, data);
        }
    }
    auto RemoveTokenPairSequencePosition = [&](LinkedList<Token>::Iterator position)
    {
        if(position == m_tokenizedInputSequence.end())
        {
            return;
        }
        auto nextPosition = position;
        ++nextPosition;
        if(nextPosition == m_tokenizedInputSequence.end())
        {
            return;
        }
        auto & left = *position;
        auto & right = *nextPosition;
        const TokenPair pair(left, right);
        auto & countAndPosition = m_tokenPairCountAndPositions[pair];
        --countAndPosition.count;
        auto & positions = countAndPosition.positions;
        positions.erase(position);
        QueueCandidate(pair, countAndPosition);
    };
    auto AddTokenPairSequencePosition = [&](LinkedList<Token>::Iterator position)
    {
        if(position == m_tokenizedInputSequence.end())
        {
            return;
        }
        auto nextPosition = position;
        ++nextPosition;
        if(nextPosition == m_tokenizedInputSequence.end())
        {
            return;
        }
        auto & left = *position;
        auto & right = *nextPosition;
        const TokenPair pair(left, right);
        auto & countAndPosition = m_tokenPairCountAndPositions[pair];
        ++countAndPosition.count;
        auto & positions = countAndPosition.positions;
        positions.emplace(position);
        QueueCandidate(pair, countAndPosition);
    };
    // todo: think of what this should be
    while(m_tokenPairReplacements.size() + 512 < Configuration::TokenDictionary)
    {
        while(!candidates.empty())
        {
            const auto& candidate = candidates.top();
            const auto current = m_tokenPairCountAndPositions.find(candidate.pair);
            const auto version = versions.find(candidate.pair);
            if(current == m_tokenPairCountAndPositions.end() ||
               current->second.positions.empty() ||
               version == versions.end() || version->second != candidate.version)
            {
                candidates.pop();
                continue;
            }
            break;
        }
        if(candidates.empty())
        {
            break;
        }
        const auto tokenPair = candidates.top().pair;
        candidates.pop();
        const auto & left = tokenPair.left;
        const auto & right = tokenPair.right;
        auto replacementIt = m_tokenPairReplacements.find(tokenPair);
        if(replacementIt == m_tokenPairReplacements.end())
        {
            Token newToken = m_nextUnusedToken++;
            m_tokenPairReplacements[tokenPair] = newToken;
            m_tokenReplacements.emplace(newToken, tokenPair);
        }
        else
        {
            // an existing replacement for this token pair already exists, weird, but let's use it
        }
        const auto & replacement = m_tokenPairReplacements[tokenPair];
        while(true)
        {
            auto selectedPair = m_tokenPairCountAndPositions.find(tokenPair);
            if(selectedPair == m_tokenPairCountAndPositions.end() || selectedPair->second.positions.empty())
            {
                break;
            }
            auto position = *selectedPair->second.positions.begin();
            auto nextPosition = position;
            ++nextPosition;
            if(nextPosition != m_tokenizedInputSequence.end() && *position == left && *nextPosition == right)
            {
                auto prevPosition = position;
                --prevPosition;
                auto nextNextPosition = nextPosition;
                ++nextNextPosition;
                RemoveTokenPairSequencePosition(prevPosition);
                RemoveTokenPairSequencePosition(position);
                RemoveTokenPairSequencePosition(nextPosition);
                position = m_tokenizedInputSequence.ReplaceTwo(position, replacement);
                AddTokenPairSequencePosition(prevPosition);
                AddTokenPairSequencePosition(position);
            }
        }
    }
}

void Tokenizer::CalculateTokenRelationships()
{
    TIMER;
    // process all the tokens
    for(auto position = m_tokenizedInputSequence.begin();position != m_tokenizedInputSequence.end(); ++position)
    {
        const auto token = *position;
        auto & allRelations = m_tokenRelationships[token];
        auto depthPosition = position;
        for(size_t depth=0; depth<Configuration::RelationDepth; ++depth)
        {
            --depthPosition;
            if(depthPosition == m_tokenizedInputSequence.end())
            {
                break;
            }
            auto & relation = allRelations[depth];
            relation[*depthPosition]++;
        }
    }
}

std::string Tokenizer::GetTokenTextSequence(const Token & token) const
{
    TIMER;
    std::string text;
    std::vector<Token> pending{token};
    while(!pending.empty())
    {
        const Token current = pending.back();
        pending.pop_back();

        if(current.IsCharacter())
        {
            if(current.IsSeparator() && !text.empty())
            {
                text.push_back(' ');
            }
            text.push_back(current.GetCharacter());
            continue;
        }

        const auto expansion = m_tokenReplacements.find(current);
        if(expansion == m_tokenReplacements.end())
        {
            throw std::logic_error("Tokenizer cannot expand a token without a replacement pair");
        }
        pending.push_back(expansion->second.right);
        pending.push_back(expansion->second.left);
    }
    return text;
}
