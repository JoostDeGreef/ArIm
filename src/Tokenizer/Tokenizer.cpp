#include <algorithm>
#include <cassert>
#include <deque>
#include <map>
#include <unordered_set>
#include <set>
#include <stdexcept>

#include "Tokenizer.h"

using namespace std;

Tokenizer::Tokenizer()
    : m_tokenizedInputSequence()
    , m_nextUnusedToken(512)
    , m_tokenPairCountAndPositions()
    , m_tokenPairReplacements()
    , m_tokenReplacements()
    , m_tokenRelationships()
{}

Tokenizer::Tokenizer(SQLite::DB & db)
    : Tokenizer()
{
    LoadDataFromDB(db);
}

void Tokenizer::Process(const std::string& input)
{
    // WARNING:
    //   this is just a placeholder of what should be executed,
    //   not in any way proper or ready for review yet.
    auto firstTokenToUpdate = m_tokenizedInputSequence.Last();
    FeedInput(input);
    UpdateTokenPairCountAndPositions(firstTokenToUpdate);
    ApplyExistingTokenPairReplacements();
    OptimizeTokenLength();
    CalculateTokenRelationships();
}

void Tokenizer::FeedInput(const std::string& input)
{
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
    if(m_tokenPairCountAndPositions.empty())
    {
        return;
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
        auto & countAndPosition = m_tokenPairCountAndPositions[TokenPair(left, right)];
        --countAndPosition.count;
        auto & positions = countAndPosition.positions;
        positions.erase(position);
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
        auto & countAndPosition = m_tokenPairCountAndPositions[TokenPair(left, right)];
        ++countAndPosition.count;
        auto & positions = countAndPosition.positions;
        positions.emplace(position);
    };
    // todo: think of what this should be
    while(m_tokenPairReplacements.size() + 256 < Configuration::TokenDictionary)
    {
        auto iterMax = m_tokenPairCountAndPositions.end();
        for(auto iter = m_tokenPairCountAndPositions.begin(); iter != m_tokenPairCountAndPositions.end(); ++iter)
        {
            if(!iter->second.positions.empty() &&
               (iterMax == m_tokenPairCountAndPositions.end() || iter->second.count > iterMax->second.count))
            {
                iterMax = iter;
            }
        }
        if(iterMax == m_tokenPairCountAndPositions.end())
        {
            break;
        }
        const auto tokenPair = iterMax->first;
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
