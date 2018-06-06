import util, math, random
from collections import defaultdict
from util import ValueIteration

############################################################
# Problem 2a


# If you decide 2a is true, prove it in blackjack.pdf and put "return None" for
# the code blocks below.  If you decide that 2a is false, construct a counterexample.
class CounterexampleMDP(util.MDP):
    def startState(self):
        # BEGIN_YOUR_CODE (our solution is 1 line of code, but don't worry if you deviate from this)
        return 0
        # END_YOUR_CODE

    # Return set of actions possible from |state|.
    def actions(self, state):
        # BEGIN_YOUR_CODE (our solution is 1 line of code, but don't worry if you deviate from this)
        return [1]
        # END_YOUR_CODE

    # Return a list of (newState, prob, reward) tuples corresponding to edges
    # coming out of |state|.
    def succAndProbReward(self, state, action):
        # BEGIN_YOUR_CODE (our solution is 1 line of code, but don't worry if you deviate from this)
        return [(state + 1, 0.9, 100), (state - 1, 0.1,
                                        -20)] if state == 0 else []
        # END_YOUR_CODE

    def discount(self):
        # BEGIN_YOUR_CODE (our solution is 1 line of code, but don't worry if you deviate from this)
        return 1
        # END_YOUR_CODE


############################################################
# Problem 3a


class BlackjackMDP(util.MDP):
    def __init__(self, cardValues, multiplicity, threshold, peekCost):
        """
        cardValues: array of card values for each card type
        multiplicity: number of each card type
        threshold: maximum total before going bust
        peekCost: how much it costs to peek at the next card
        """
        self.cardValues = cardValues
        self.multiplicity = multiplicity
        self.threshold = threshold
        self.peekCost = peekCost

    # Return the start state.
    # Look at this function to learn about the state representation.
    # The first element of the tuple is the sum of the cards in the player's
    # hand.
    # The second element is the index (not the value) of the next card, if the player peeked in the
    # last action.  If they didn't peek, this will be None.
    # The final element is the current deck.
    def startState(self):
        return (0, None, (self.multiplicity, ) * len(self.cardValues)
                )  # total, next card (if any), multiplicity for each card

    # Return set of actions possible from |state|.
    # You do not need to modify this function.
    # All logic for dealing with end states should be done in succAndProbReward
    def actions(self, state):
        return ['Take', 'Peek', 'Quit']

    # Return a list of (newState, prob, reward) tuples corresponding to edges
    # coming out of |state|.  Indicate a terminal state (after quitting or
    # busting) by setting the deck to None.
    # When the probability is 0 for a particular transition, don't include that
    # in the list returned by succAndProbReward.
    def succAndProbReward(self, state, action):
        # BEGIN_YOUR_CODE (our solution is 53 lines of code, but don't worry if you deviate from this)
        totalCardValueInHand, nextCardIndexIfPeeked, deckCardCounts = state
        if deckCardCounts == None:
            return []
        numCardsInDeck = float(sum(list(deckCardCounts)))

        if nextCardIndexIfPeeked != None and action == 'Take':
            totalCardValueInHand += self.cardValues[nextCardIndexIfPeeked]
            if numCardsInDeck == 1 or totalCardValueInHand > self.threshold:
                nextdeckCardCounts = None
                reward = totalCardValueInHand if totalCardValueInHand <= self.threshold else 0
            else:
                nextdeckCardCounts = list(deckCardCounts)
                nextdeckCardCounts[nextCardIndexIfPeeked] -= 1
                nextdeckCardCounts = tuple(nextdeckCardCounts)
                reward = 0
            return [((totalCardValueInHand, None, nextdeckCardCounts), 1,
                     reward)]
        elif nextCardIndexIfPeeked != None and action == 'Peek':
            return []
        elif action == 'Take':
            results = []
            for idx, cnt in enumerate(deckCardCounts):
                if cnt > 0:
                    nextTotalValue = totalCardValueInHand + self.cardValues[idx]
                    if numCardsInDeck == 1 or nextTotalValue > self.threshold:
                        nextdeckCardCounts = None
                        reward = nextTotalValue if nextTotalValue <= self.threshold else 0
                    else:
                        nextdeckCardCounts = list(deckCardCounts)
                        nextdeckCardCounts[idx] -= 1
                        nextdeckCardCounts = tuple(nextdeckCardCounts)
                        reward = 0
                    newState = (nextTotalValue, None, nextdeckCardCounts)
                    prob = cnt / numCardsInDeck
                    results.append((newState, prob, reward))
            return results

        elif action == 'Peek':
            return [((totalCardValueInHand, idx, deckCardCounts),\
                cnt / numCardsInDeck, -self.peekCost) \
                for idx, cnt in enumerate(deckCardCounts) if cnt > 0]

        elif action == 'Quit':
            return [((totalCardValueInHand, None, None), 1,
                     totalCardValueInHand)]
        # END_YOUR_CODE

    def discount(self):
        return 1


############################################################
# Problem 3b


def peekingMDP():
    """
    Return an instance of BlackjackMDP where peeking is the optimal action at
    least 10% of the time.
    """
    # BEGIN_YOUR_CODE (our solution is 2 lines of code, but don't worry if you deviate from this)
    return BlackjackMDP(cardValues=[4,5,6,20], multiplicity=1,\
                        threshold=20, peekCost=1)
    # END_YOUR_CODE


############################################################
# Problem 4a: Q learning


# Performs Q-learning.  Read util.RLAlgorithm for more information.
# actions: a function that takes a state and returns a list of actions.
# discount: a number between 0 and 1, which determines the discount factor
# featureExtractor: a function that takes a state and action and returns a list of (feature name, feature value) pairs.
# explorationProb: the epsilon value indicating how frequently the policy
# returns a random action
class QLearningAlgorithm(util.RLAlgorithm):
    def __init__(self,
                 actions,
                 discount,
                 featureExtractor,
                 explorationProb=0.2):
        self.actions = actions
        self.discount = discount
        self.featureExtractor = featureExtractor
        self.explorationProb = explorationProb
        self.weights = defaultdict(float)
        self.numIters = 0

    # Return the Q function associated with the weights and features
    def getQ(self, state, action):
        score = 0
        for f, v in self.featureExtractor(state, action):
            score += self.weights[f] * v
        return score

    # This algorithm will produce an action given a state.
    # Here we use the epsilon-greedy algorithm: with probability
    # |explorationProb|, take a random action.
    def getAction(self, state):
        self.numIters += 1
        if random.random() < self.explorationProb:
            return random.choice(self.actions(state))
        else:
            return max((self.getQ(state, action), action)
                       for action in self.actions(state))[1]

    # Call this function to get the step size to update the weights.
    def getStepSize(self):
        return 1.0 / math.sqrt(self.numIters)

    # We will call this function with (s, a, r, s'), which you should use to update |weights|.
    # Note that if s is a terminal state, then s' will be None.  Remember to check for this.
    # You should update the weights using self.getStepSize(); use
    # self.getQ() to compute the current estimate of the parameters.
    def incorporateFeedback(self, state, action, reward, newState):
        # BEGIN_YOUR_CODE (our solution is 12 lines of code, but don't worry if you deviate from this)
        prediction = self.getQ(state, action)
        if newState == None:
            target = reward
        else:
            target = reward + self.discount * \
                 max(self.getQ(newState, _action) for _action in self.actions(newState))
        for f, v in self.featureExtractor(state, action):
            self.weights[f] -= self.getStepSize() * (prediction - target) * v
        # END_YOUR_CODE


# Return a singleton list containing indicator feature for the (state, action)
# pair.  Provides no generalization.
def identityFeatureExtractor(state, action):
    featureKey = (state, action)
    featureValue = 1
    return [(featureKey, featureValue)]


############################################################
# Problem 4b: convergence of Q-learning
# Small test case
smallMDP = BlackjackMDP(
    cardValues=[1, 5], multiplicity=2, threshold=10, peekCost=1)

# Large test case
largeMDP = BlackjackMDP(
    cardValues=[1, 3, 5, 8, 10], multiplicity=3, threshold=40, peekCost=1)
largeMDP.computeStates()


def MDP_simulate(mdp=smallMDP,
                 file_name='small_result.txt',
                 FeatureExtractor=identityFeatureExtractor):
    f = open(file_name, 'w')
    mdp.computeStates()
    states = list(mdp.states)

    QLA_results = {}
    QLA = QLearningAlgorithm(mdp.actions, mdp.discount(), \
                FeatureExtractor)
    util.simulate(mdp, QLA, 30000)
    QLA.explorationProb = 0
    for state in states:
        QLA_results[state] = QLA.getAction(state)
    VI = util.ValueIteration()
    VI.solve(mdp)
    VI_results = VI.pi
    f.write('\tstate \tQLA_policy\tVI_policy\n')
    f.write('=======================================\n')
    state_cnt = 0
    false_state_cnt = 0
    for state in list(VI_results.keys()):
        state_cnt += 1 if state != None else 0
        if state != None and VI_results[state] != QLA_results[state]:
            f.write(str(state)+ '\t' + str(QLA_results[state]) +\
                '\t' + str(VI_results[state])+'\n')
            false_state_cnt += 1
    f.write('=======================================\n')
    f.write('Different rate : '+str(false_state_cnt)+' / '+\
            str(state_cnt)+' = '+str(float(false_state_cnt)/float(state_cnt)))
    f.close()


#MDP_simulate(smallMDP, 'small_result.txt')
#MDP_simulate(largeMDP, 'large_result.txt')

############################################################
# Problem 4c: features for Q-learning.


# You should return a list of (feature key, feature value) pairs (see
# identityFeatureExtractor()).
# Implement the following features:
# - indicator on the total and the action (1 feature).
# - indicator on the presence/absence of each card and the action (1 feature).
#       Example: if the deck is (3, 4, 0 , 2), then your indicator on the presence of each card is (1,1,0,1)
#       Only add this feature if the deck != None
# - indicator on the number of cards for each card type and the action (len(counts) features).  Only add these features if the deck != None
def blackjackFeatureExtractor(state, action):
    total, nextCard, counts = state
    # BEGIN_YOUR_CODE (our solution is 9 lines of code, but don't worry if you deviate from this)
    results = []
    results.append(((total, action), 1))
    if counts != None:
        presence = [1 if count > 0 else 0 for count in counts]
        presence = tuple(presence)
        results.append(((presence, action), 1))
        for idx, count in enumerate(counts):
            results.append(((idx, count, action), 1))
    return results
    # END_YOUR_CODE


#MDP_simulate(largeMDP, 'large_modified_result.txt', blackjackFeatureExtractor)
############################################################
# Problem 4d: What happens when the MDP changes underneath you?!

# Original mdp
originalMDP = BlackjackMDP(
    cardValues=[1, 5], multiplicity=2, threshold=10, peekCost=1)

# New threshold
newThresholdMDP = BlackjackMDP(
    cardValues=[1, 5], multiplicity=2, threshold=15, peekCost=1)


def ChangeMDP():
    f = open('prob_4d_results.txt', 'w')
    newThresholdMDP.computeStates()
    states = list(newThresholdMDP.states)

    VI = util.ValueIteration()
    VI.solve(originalMDP)
    VI_results = VI.pi
    orgin_algorithm = util.FixedRLAlgorithm(VI.pi)
    reward_list1 = util.simulate(newThresholdMDP, orgin_algorithm)
    reward1 = float(sum(reward_list1)) / len(reward_list1)

    QLA_results = {}
    QLA = QLearningAlgorithm(newThresholdMDP.actions, \
        newThresholdMDP.discount(), blackjackFeatureExtractor)
    util.simulate(newThresholdMDP, QLA, 3000)
    QLA.explorationProb = 0
    reward_list2 = util.simulate(newThresholdMDP, QLA)
    reward2 = float(sum(reward_list2)) / len(reward_list2)

    for state in states:
        QLA_results[state] = QLA.getAction(state)

    f.write('orgin reward: ' + str(reward1)+'\n Q-Learning reward: '+\
          str(reward2)+'\n')
    f.write('\tstate \torgin_policy\tQLA_policy\n')
    f.write('=======================================\n')
    state_cnt = 0
    false_state_cnt = 0
    for state in list(VI_results.keys()):
        state_cnt += 1 if state != None else 0
        if state != None and VI_results[state] != QLA_results[state]:
            f.write(str(state)+'\t' + str(VI_results[state])+\
                '\t' + str(QLA_results[state]) +'\n')
            false_state_cnt += 1
    f.write('=======================================\n')
    f.write('Different rate : '+str(false_state_cnt)+' / '+\
          str(state_cnt)+' = '+str(float(false_state_cnt)/float(state_cnt)))
    f.close()


#ChangeMDP()
