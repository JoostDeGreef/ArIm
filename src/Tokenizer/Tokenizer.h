#pragma once

#include <array>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "SQLiteDB.h"
#include "CountAndPosition.h"
#include "LinkedList.h"
#include "Token.h"
#include "TokenPair.h"

/*
 * Tokenizer class for tokenizing input text into tokens. 
 *
 * Normal operation steps:
 *   1. Optional: load existing tokenisation from db
 *   2. Feed data to 'Process()'
 *   3. Store the results to db
 */
class Tokenizer 
{   
public:
    Tokenizer();
    Tokenizer(sqlite::DB & db);
    // This class stores a lot of data, so we don't want to allow copying or moving it around.
    Tokenizer(const Tokenizer&) = delete;
    Tokenizer(Tokenizer&&) = delete;
    Tokenizer& operator=(const Tokenizer&) = delete;
    Tokenizer& operator=(Tokenizer&&) = delete;

    // Add text to the tokenizer. can be called repeatedly
    void Process(const std::string& input); 

    // Store tokenizer state to a database
    void StoreTokenizerToDB(sqlite::DB & db) const;

private:
    struct Configuration
    {
        static constexpr size_t Version         = 1;    // version for the db. increase if anything breaking gets introduced.
        static constexpr size_t RelationDepth   = 5;    // number of tokens to keep a relationship with
        static constexpr size_t TokenDictionary = 1024; // number of tokens to build a dictionary for
    };

    // input data split in tokens
    LinkedList<Token> m_tokenizedInputSequence;

    // next unused token value, used to create new tokens when merging token pairs
    Token m_nextUnusedToken;

    // count and position of each tokenpair in 'tokens'
    std::unordered_map<TokenPair,CountAndPosition,TokenPair::HashFunction> m_tokenPairCountAndPositions;

    // keep track of what token pair was replaced with which token
    // needed for reconstruction of input sequence
    std::unordered_map<TokenPair,Token,TokenPair::HashFunction> m_tokenPairReplacements;

    // map with a counted dictionary with the preceding tokens listed for each token
    std::unordered_map<Token,std::array<std::unordered_map<Token,size_t,Token::HashFunction>,Configuration::RelationDepth>,Token::HashFunction> m_tokenRelationships;

    // Add text to the tokenizer. can be called repeatedly
    // This adds tokens to the back of tokenizedInputSequence and updates the tokenCounts map
    void FeedInput(const std::string& input); 

    // void update the count and position of a tokenpair starting at a given position in the sequence
    void UpdateTokenPairCountAndPositions(LinkedList<Token>::Iterator position);

    // used the existing tokenPairReplacements on the tokenizedInputSequence
    void ApplyExistingTokenPairReplacements();

    // join tokens until the maximum (used) token count is reached or no more text is available
    void OptimizeTokenLength();

    // determine the token relationships
    void CalculateTokenRelationships();

    // load the tokenizer state from a database
    void LoadDataFromDB(sqlite::DB & db);
};
