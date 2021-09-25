# Readme
## 1 To run the crawler
There is only one .py file in the directory. It contains all the code needed to run my crawler. 

Before running the crawler, you should check the requirements.txt file to get all the packets needed. 

When running the crawler, you should input 4 times to initial the crawler. 
+ you should input your key words, e.g. "brooklyn union". 
+ you should input your goal of successful urls to crawl, e.g. "10000".
+ you should input the number of threads you want, e.g. "50". 
+ you should input the mode of the crawler, "p" for priority mode and all other input for regular mode. 

After running the crawler, logs will be appended to log.log file in the directory (new one will be created if needed). 

There are 4 example logs and 1 requirements.txt files in the directory, too. 

## 2 About the Code

The main component of the code is the class "crawler". 

Class "crawler" contains class variables such as waiting_list, novelty, importance, visited, etc. 
+ The waiting_list is a PriorityQueue for priority mode and Queue for regular mode. It stores all the crawled links that have not been visited. 
+ The novelty and importance dictionary store the site novelty and url importance in priority mode.
+ The visited set store all the visited(at least tried) urls to prevent visiting an url more than once. 
+ The class also has other class variables to store basic statistics that have little effect on the function of the class. 

Class "crawler" has five class method: init, get_next, download_url, parse_html, and carwl. 
+ Method init will initial all the class variables and call a function to get seed urls from google.com.  
+ Method get_next always try to find the best next url to crawl. It will drop visited urls and update url's novelty in priority mode. 
+ Method download_url will use requests.get to download html file from the url given by get_next. It will call a function to check robots.txt files and catch all kinds of errors to log them in the log file. 
+ Method parse_html will use BeautifulSoup to parse the html downloaded by download_url. It handles different types of links, update sites' importance in priority mode, and put newly found urls in the waiting_list. Notice that the PriorityQueue cannot be updated so we will add same urls with different priority score to the queue. However, we should not worry about it because we will only visit an url once. 
+ Method crawl will connect all the methods above and log successful information. It is the method that will be called by threads. 

Apart from the class "crawler", I have a function to check the robots.txt files and a function to get seed urls from google. 
+ Function check_robot will use RobotFileParser to read and parse robots.txt files. It will check whether we can fetch the url or not. 
+ Function get_seeds will pretend to be a user to visit google.com with given queries and return top results. We have to do this because google don't allow regular crawler visit. This function is modified from code of a blog: https://cloud.tencent.com/developer/article/1581779. 

Lastly, in order to utilize multi-threading to speed up crawling, I have class MyThread inherited from threading.Thread. It will be called by the main function and enable us to crawl with multiple threads. 


## 3 Bugs or Non-working Features

+ The input part of the main function is not carefully designed. So please input as guided. 
+ There are potential exceptions not handled when checking robots.txt file or get html file using requests.get. So some of the threads may die during the crawling (very rare). 
+ There are potential format errors when logger trying to log complicated languages (such as Sanskrit). 

## 4 Special features

My crawler can keep crawling for a long time. Actually, I wonder if the crawler will only stop working after it takes up all my RAM space. 