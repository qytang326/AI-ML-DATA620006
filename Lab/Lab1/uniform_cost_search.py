import heapq


class PriorityQueue(object):
    def __init__(self):
        self.heap = []
        self.count = 0

    def push(self, item, priority):
        entry = (priority, self.count, item)
        heapq.heappush(self.heap, entry)
        self.count += 1

    def pop(self):
        (_, _, item) = heapq.heappop(self.heap)
        return item

    def isEmpty(self):
        return len(self.heap) == 0

    def update(self, item, priority):
        for index, (p, c, i) in enumerate(self.heap):
            if i == item:
                if p <= priority:
                    break
                del self.heap[index]
                self.heap.append((priority, c, item))
                heapq.heapify(self.heap)
                break
        else:
            self.push(item, priority)


class node:
    """define node"""

    def __init__(self, state, parent, path_cost, action):
        self.state = state
        self.parent = parent
        self.path_cost = path_cost
        self.action = action


class problem:
    """searching problem"""

    def __init__(self, initial_state, actions):
        self.initial_state = initial_state
        self.actions = actions
        # 可以在这里随意添加代码或者不加

    def search_actions(self, state):
        #raise Exception('获取state的所有可行的动作')
        state_action = []
        for i in self.actions:
            if i[0] == state:
                state_action.append(i)
        return state_action

    def solution(self, node):
        #raise Exception('获取从初始节点到node的路径')
        position = node
        path = ["Start"]
        while position.state != self.initial_state:
            path.insert(1, position.state)
            position = position.parent
        return path

    def transition(self, state, action):
        #raise Exception('节点的状态（名字）经过action转移之后的状态（名字）')
        if action[0] == state:
            return action[1]
        else:
            return "Unreachable"

    def goal_test(self, state):
        #raise Exception('判断state是不是终止节点')
        if state == "Goal":
            return True
        else:
            return False

    def step_cost(self, state1, action, state2):
        #raise Exception('获得从state1到通过action到达state2的cost')
        if (state1 == action[0]) & (state2 == action[1]) & (
                action in self.actions):
            return int(action[2])
        else:
            return "Unreachable"

    def child_node(self, node_begin, action):
        #raise Exception('获取从起始节点node_begin经过action到达的node')
        ChildNode = node(
            self.transition(node_begin.state,
                            action), node_begin, node_begin.path_cost +
            self.step_cost(node_begin.state, action,
                           self.transition(node_begin.state, action)), action)
        return ChildNode


def UCS(problem):
    node_test = node(problem.initial_state, '', 0, '')
    frontier = PriorityQueue()
    frontier.push(node_test, node_test.path_cost)
    explored = []
    #raise Exception('进行循环')
    while not frontier.isEmpty():
        node_test = frontier.pop()
        if problem.goal_test(node_test.state):
            return problem.solution(node_test)
        explored.append(node_test.state)
        for action in problem.search_actions(node_test.state):
            ChildNode = problem.child_node(node_test, action)
            if ChildNode.state not in explored:
                frontier.update(ChildNode, ChildNode.path_cost)
    return "Unreachable"


def main():
    actions = []
    while True:
        a = input().strip()
        if a != 'END':
            a = a.split()
            actions.append(a)
        else:
            break

    graph_problem = problem('Start', actions)
    try:
        answer = UCS(graph_problem)
    except:
        answer = "Unreachable"
    s = "->"
    if answer == 'Unreachable':
        print(answer)
    else:
        path = s.join(answer)
        print(path)


if __name__ == '__main__':
    main()
