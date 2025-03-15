#include <iostream>
#include <iomanip>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <fstream>
#include <sstream>
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace std;

// =============== 全局配置 ===============
const int MAX_SIZE = 50;       // 最大支持地图尺寸
int map[MAX_SIZE][MAX_SIZE];   // 地图数据：0-未扫，-1-障碍，1→，2↓，3←，4↑，5已扫
int rows, cols;                // 地图实际行数和列数
int cleaned_count = 0;         // 已清扫区域计数器
vector<pair<int, int> > obstacles; // 障碍物坐标容器
clock_t start_time;            // 记录开始时间

// =============== 函数声明 ===============

void initializeMap();          // 地图初始化
void updateMap();              // 更新清扫后的地图状态
void clearScreen();            // 清屏函数
void delay(int ms);            // 延时函数
void displayMap();             // 显示地图和统计信息
void setupObstacles();         // 设置障碍物
void saveMap(const string& filename); //保存地图
void loadMap(const string& filename); //加载地图
void cleaningDFS(int x, int y, int from_dir); // DFS清扫算法
void showMenu();               // 显示主菜单
void updateDynamicObstacles(); //移动动态障碍物

// =============== 函数实现 ===============

/**
 * 初始化地图数据
 * 功能：将所有区域标记为未清扫状态
 */
void initializeMap()
{
	for (int i = 0; i < MAX_SIZE; ++i)
	{
		for (int j = 0; j < MAX_SIZE; ++j)
		{
			map[i][j] = 0;
		}
	}
}

/**
 * 更新清扫后的地图状态
 * 功能：将已清扫区域标记为已清扫状态
 */
void updateMap()
{
	for (int i = 0; i < MAX_SIZE; ++i)
	{
		for (int j = 0; j < MAX_SIZE; ++j)
		{
			if (map[i][j] == 1 || map[i][j] == 2 || map[i][j] == 3 || map[i][j] == 4)
				map[i][j] = 5;
		}
	}
}

/**
 * 清屏函数（跨平台实现）
 * Windows使用cls命令，Linux/Mac使用clear命令
 */
void clearScreen()
{
#ifdef _WIN32
	system("cls");
#else
	system("clear");
#endif
}

/**
 * 延时函数（跨平台实现）
 * 功能： 延时的毫秒数
 */
void delay(int ms)
{
#ifdef _WIN32
	Sleep(ms);
#else
	usleep(ms * 1000); // 将毫秒转换为微秒
#endif
}

// 动态障碍物结构体
struct DynamicObstacle
{
	int x, y; // 障碍物坐标
};

// 动态障碍物容器
vector<DynamicObstacle> dynamicObstacles;

/**
 * 显示地图和统计信息
 * 功能：实时显示清扫进度、路径方向、时间统计和覆盖率
 */
void displayMap()
{
	clearScreen();

	// === 头部信息 ===
	cout << "====== 扫地机器人实时监控系统 ======\n";
	cout << "地图尺寸: " << rows << "x" << cols << '\n';
	cout << "  障碍物: " << obstacles.size() << "个" << '\n';

	// === 时间计算 ===
	double elapsed = (double)(clock() - start_time) / CLOCKS_PER_SEC;
	double remaining = (cleaned_count > 0) ?
		(elapsed * (rows * cols - obstacles.size() - cleaned_count) / cleaned_count) : 0;

	// === 统计信息 ===
	cout << "已用时间: " << fixed << setprecision(1) << elapsed << "s" << '\n';
	cout << "预估剩余: " << remaining << "s" << '\n';
	cout << "  覆盖率: " << fixed << setprecision(1)
		<< (cleaned_count * 100.0) / (rows * cols - (int)obstacles.size()) << "%\n";

	// === 图例说明 ===
	cout << "    图例：■ 障碍 ★ 动态障碍\n";
	cout << "          ○ 未扫 ●  已扫\n";
	cout << "          → ↓ ← ↑   移动方向\n";

	// === 地图绘制 ===
	for (int i = 1; i <= rows; ++i)
	{
		for (int j = 1; j <= cols; ++j)
		{
			// 检查是否是动态障碍物
			bool isDynamic = false;
			for (const auto& obstacle : dynamicObstacles)
			{
				if (i == obstacle.x && j == obstacle.y)
				{
					cout << " ★";
					isDynamic = true;
					break;
				}
			}
			if (isDynamic) continue;
			// 根据地图数据选择显示符号
			switch (map[i][j])
			{
			case -1:// 障碍物
				cout << " ■";
				break;
			case 0:// 未清扫区域
				cout << " ○";
				break;
			case 1:// 向右清扫
				cout << " →";
				break;
			case 2:// 向下清扫
				cout << " ↓";
				break;
			case 3:// 向左清扫
				cout << " ←";
				break;
			case 4:// 向上清扫
				cout << " ↑";
				break;
			default:// 已清扫区域
				cout << " ●";
			}
		}
		cout << '\n';
	}
	cout.flush(); // 强制刷新输出缓冲区

}

/**
 * 深度优先清扫算法
 * @param x,y 当前坐标
 * @param from_dir 移动来源方向（用于显示路径箭头）
 */
void cleaningDFS(int x, int y, int from_dir)
{
	// 边界检查：超出地图范围或遇到障碍/已扫区域时返回
	if (x < 1 || x > rows || y < 1 || y > cols || map[x][y] != 0) return;

	// 更新当前区域状态
	map[x][y] = from_dir;  // 记录移动方向
	cleaned_count++;       // 增加已清扫计数

	// 显示更新（每步都刷新）
	displayMap();
	delay(500);  // 控制显示速度

	// 更新动态障碍物位置
	updateDynamicObstacles();
	//更新已清扫地图状态
	updateMap();
	// 按优先级探索四个方向
	cleaningDFS(x, y + 1, 1);  // 向右移动，标记来自左侧(→)
	cleaningDFS(x + 1, y, 2);  // 向下移动，标记来自上方(↓)
	cleaningDFS(x, y - 1, 3);  // 向左移动，标记来自右侧(←)
	cleaningDFS(x - 1, y, 4);  // 向上移动，标记来自下方(↑)

	//显示最后一个清扫点的状态
	displayMap();
}

/**
 * 设置障碍物
 * 功能：接收用户输入，记录障碍物坐标
 */
void setupObstacles()
{
	int count, x, y;
	cout << "请输入障碍物数量: ";
	cin >> count;

	while (count-- > 0)
	{
		cout << "输入坐标(x y，范围1-" << rows << " 1-" << cols << "): ";
		cin >> x >> y;
		if (x >= 1 && x <= rows && y >= 1 && y <= cols)
		{
			map[x][y] = -1;
			obstacles.push_back(make_pair(x, y));
			// 随机选择部分障碍物作为动态障碍物
			if (rand() % 2 == 0) //50%的概率
			{
				dynamicObstacles.push_back({ x, y });
			}
		}
	}
	cout.flush(); // 强制刷新输出缓冲区
}

/**
 * 更新动态障碍物的位置
 * 功能：随机移动动态障碍物，并更新地图数据
 **/
void updateDynamicObstacles()
{
	for (auto& obstacle : dynamicObstacles)
	{
		int direction = rand() % 4; // 随机选择方向：0-右，1-下，2-左，3-上
		int newX = obstacle.x, newY = obstacle.y;

		switch (direction)
		{
		case 0:
			newY++;
			break; // 向右移动
		case 1:
			newX++;
			break; // 向下移动
		case 2:
			newY--;
			break; // 向左移动
		case 3:
			newX--;
			break; // 向上移动
		}

		// 检查新位置是否合法
		if (newX >= 1 && newX <= rows && newY >= 1 && newY <= cols && map[newX][newY] == 0)
		{
			map[obstacle.x][obstacle.y] = 0; // 原障碍物位置设置为未清扫
			obstacle.x = newX;
			obstacle.y = newY;
			map[obstacle.x][obstacle.y] = -1; // 更新新障碍物位置
		}
	}
}

/**
 * 保存地图数据
 * 功能：将地图数据保存到文件
 * 注意：文件格式为：.lbzsmap
 */
void saveMap(const string& filename)
{
	ofstream outFile(filename); // 打开文件
	if (!outFile)
	{
		cerr << "无法保存地图文件: " << filename << '\n';
		return;
	}

	// 保存地图尺寸
	outFile << rows << " " << cols << '\n';

	// 保存障碍物
	for (const auto& obstacle : obstacles)
	{
		outFile << obstacle.first << " " << obstacle.second << '\n';
	}

	outFile.close();
	cout << "地图已保存到文件: " << filename << '\n';
}

/**
 * 加载地图数据
 * 功能：从文件加载地图数据
 * 注意：文件格式为：.lbzsmap
 */
void loadMap(const string& filename)
{
	ifstream inFile(filename); // 打开文件
	if (!inFile)
	{
		cerr << "无法加载地图文件: " << filename << '\n';
		return;
	}

	// 清空当前地图和障碍物
	initializeMap();
	obstacles.clear();

	// 读取地图尺寸
	inFile >> rows >> cols;

	// 读取障碍物
	int x, y;
	while (inFile >> x >> y)
	{
		if (x >= 1 && x <= rows && y >= 1 && y <= cols)
		{
			map[x][y] = -1;
			obstacles.push_back(make_pair(x, y));
		}
	}

	inFile.close();
	cout << "地图已从文件加载: " << filename << '\n';
	cout.flush(); // 强制刷新输出缓冲区
}

/**
 * 显示主菜单
 * 功能：提供用户交互界面
 */
void showMenu()
{
	clearScreen();
	cout << "===== 主菜单 =====\n";
	cout << "| 1. 设置地图尺寸 |\n";
	cout << "| 2. 添加障碍物   |\n";
	cout << "| 3. 开始清扫     |\n";
	cout << "| 4. 保存地图     |\n";
	cout << "| 5. 加载地图     |\n";
	cout << "| 6. 退出程序     |\n";
	cout << "-------------------\n";
	cout << " 请输入选项: ";
	cout.flush(); // 强制刷新输出缓冲区
}

/**
 * 主函数
 */
int main()
{
	ios::sync_with_stdio(false);
	cin.tie(nullptr);
	int choice = -1;

	do
	{
		showMenu();
		cin >> choice;

		switch (choice)
		{
			case 1: // 设置地图尺寸
				cout << "输入地图尺寸(行 列): ";
				cin >> rows >> cols;
				initializeMap();
				// 恢复已有障碍物
				for (auto it = obstacles.begin(); it != obstacles.end(); ++it)
				{
					map[it->first][it->second] = -1;
				}
				break;

			case 2: // 添加障碍物
				if (rows == 0 || cols == 0)
				{
					cout << "请先设置地图尺寸！";
					delay(1000);
					break;
				}
				setupObstacles();
				break;

			case 3: // 开始清扫
				if (rows == 0 || cols == 0)
				{
					cout << "请先设置地图尺寸！";
					delay(1000);
					break;
				}
				cleaned_count = 0;
				start_time = clock();  // 记录开始时间
				cleaningDFS(1, 1, 1);  // 从左上角开始清扫
				cout << "\n清扫完成！总耗时: "
					<< (double)(clock() - start_time) / CLOCKS_PER_SEC << "秒\n";
				cin.ignore();  // 清除输入缓冲区
				cin.get();     // 等待用户按键
				break;

			case 4: // 保存地图
				if (rows == 0 || cols == 0)
				{
					cout << "请先设置地图尺寸！";
					delay(1000);
					break;
				}
				{
					string filename;
					cout << "输入保存文件名: ";
					cin >> filename;
					filename += ".lbzsmap"; // 自动添加文件扩展名
					saveMap(filename);
				}
				break;

			case 5: // 加载地图
			{
				string filename;
				cout << "输入加载文件名: ";
				cin >> filename;
				filename += ".lbzsmap"; // 自动添加文件扩展名
				loadMap(filename);
			}
			break;

			default: // 处理无效输入
				cin.clear(); // 清除错误状态
				cin.ignore((std::numeric_limits< streamsize >::max)(), '\n');//清除输入缓冲区
				break;
		}
	} while (choice != 6);

	return 0;
}