from queue import PriorityQueue
from queue import Queue
from bs4 import BeautifulSoup
from urllib.parse import urlparse
from urllib.parse import urljoin
from urllib.robotparser import RobotFileParser
from urllib import error
import http.client
import math
import socket
import ssl
import logging
import requests
import threading
import time


class Crawler(object):

    def __init__(self, seeds, mode):
        # saves mode
        self.mode = mode

        # saves urls fetched
        if mode == "p":
            logging.warning("crawling in priority mode")
            self.waiting_list = PriorityQueue()
            # saves site novelty
            self.novelty = {}
            # saves site importance
            self.importance = {}
        else:
            logging.warning("crawling in regular mode")
            self.waiting_list = Queue()

        # saves visited urls
        self.visited = set()
        # saves number of crawled urls
        self.number_of_urls = 0
        # saves number of all crawled links
        self.links = 0
        # saves total length of html files
        self.total_length = 0
        # saves number of times a site been visited
        self.site_count = {}

        # put seeds in the queue and give them higher priority than normal urls
        if mode == "p":
            for url in seeds:
                if url[-1] != "/":
                    url += "/"
                site = urlparse(url).netloc
                self.novelty[site] = 1
                self.importance[url] = 10
                self.waiting_list.put((-(math.log(self.importance[url]) + 1/self.novelty[site]), url))
        else:
            for url in seeds:
                self.waiting_list.put(url)

    def get_next(self):
        # get one from the queue
        if self.mode == "p":
            cur_priority, cur_url = self.waiting_list.get()
        else:
            cur_url = self.waiting_list.get()
            cur_priority = 0
        cur_site = urlparse(cur_url).netloc

        # determine if the current url has the highest priority
        while not self.waiting_list.empty():
            # if we have visited current url before, find next one
            if cur_url in self.visited:
                if self.mode == "p":
                    cur_priority, cur_url = self.waiting_list.get()
                else:
                    cur_url = self.waiting_list.get()
                cur_site = urlparse(cur_url).netloc
                continue

            if self.mode != "p":
                break

            # get next url to find if the current has higher priority
            next_priority, next_url = self.waiting_list.get()
            next_site = urlparse(next_url).netloc

            # update current url's priority
            cur_priority = -(math.log(self.importance[cur_url]) + 1 / (self.novelty[cur_site]))

            # update next url's priority
            next_priority = -(math.log(self.importance[next_url]) + 1 / (self.novelty[next_site]))

            # choose one between cur and next
            if cur_priority <= next_priority:
                # the current url has higher priority, carry on
                self.waiting_list.put((next_priority, next_url))
                break
            else:
                # the next url has higher priority, loop
                self.waiting_list.put((cur_priority, cur_url))
                cur_url = next_url
                cur_site = next_site
                cur_priority = next_priority
                continue

        # if the url is visited and we don't have next one, stop crawling
        if cur_url in self.visited:
            return None, None, None

        return cur_url, cur_site, cur_priority

    def download_url(self, cur_url, cur_site, cur_priority):
        # set the url is visited
        self.visited.add(cur_url)

        # check the robot.txt
        if not check_robot("https://" + cur_site, cur_url):
            # log unreachable urls
            logging.warning(threading.current_thread().name + ": " +
                            "priority = {}, code = None url = {}, should not be crawled due to robot.txt".format(
                                round(cur_priority, 2),
                                cur_url)
                            )
            return None

        # try to download html from url
        try:
            resp = requests.get(cur_url, timeout=1)
        except requests.exceptions.Timeout:
            # log timeouts
            logging.warning(threading.current_thread().name + ": " +
                            "priority = {}, code = None, url = {}, time out to reach".format(
                                round(cur_priority, 2),
                                cur_url)
                            )
            return None
        except requests.exceptions.ConnectionError:
            # log connection error
            logging.warning(threading.current_thread().name + ": " +
                            "priority = {}, code = None, url = {}, connection error".format(
                                round(cur_priority, 2),
                                cur_url)
                            )
            return None
        except requests.exceptions.TooManyRedirects:
            # log redirect error
            logging.warning(threading.current_thread().name + ": " +
                            "priority = {}, code = None, url = {}, too many redirects error".format(
                                round(cur_priority, 2),
                                cur_url)
                            )
            return None
        except requests.exceptions.ChunkedEncodingError:
            # log incomplete read
            logging.warning(threading.current_thread().name + ": " +
                            "priority = {}, code = None, url = {}, incomplete read error".format(
                                round(cur_priority, 2),
                                cur_url)
                            )
            return None
        except requests.exceptions.InvalidSchema:
            # log invalid schema
            logging.warning(threading.current_thread().name + ": " +
                            "priority = {}, code = None, url = {}, invalid schema error".format(
                                round(cur_priority, 2),
                                cur_url)
                            )
            return None
        except requests.exceptions.ContentDecodingError:
            # log decode error
            logging.warning(threading.current_thread().name + ": " +
                            "priority = {}, code = None, url = {}, fail to decode error".format(
                                round(cur_priority, 2),
                                cur_url)
                            )
            return None
        except UnicodeError:
            # log unicode error
            logging.warning(threading.current_thread().name + ": " +
                            "priority = {}, code = None, url = {}, label empty or too long error".format(
                                round(cur_priority, 2),
                                cur_url)
                            )
            return None
        except:
            # log rare error
            logging.warning(threading.current_thread().name + ": " +
                            "priority = {}, code = None, url = {}, some error".format(
                                round(cur_priority, 2),
                                cur_url)
                            )
            return None

        if not resp.status_code == 200:
            # log unsuccessful crawls
            logging.warning(threading.current_thread().name + ": " +
                            "priority = {}, code = {}, url = {}, cannot be crawled".format(
                                round(cur_priority, 2),
                                resp.status_code,
                                cur_url)
                            )
            return None

        if "content-type" not in resp.headers or "text/html" not in resp.headers["content-type"]:
            # log not html urls
            logging.warning(threading.current_thread().name + ": " +
                            "priority = {}, code = 200, url = {}, not html should not be parse".format(
                                round(cur_priority, 2),
                                cur_url)
                            )
            return None

        if self.mode == "p":
            # successfully crawled a url so decrease novelty of its site (by increasing)
            self.novelty[cur_site] += 1

        return resp

    def parse_html(self, resp, cur_url, cur_site):
        # parse the html downloaded
        soup = BeautifulSoup(resp.content, "html.parser")
        cnt = 0
        for link in soup.find_all("a"):
            new_url = link.get("href")
            if new_url is None:
                continue

            # clean different schemas
            new_site = urlparse(new_url).netloc
            new_path = urlparse(new_url).path
            if new_path == "" or new_path[-1] != "/":
                new_path += "/"
            new_url = "https://" + new_site + new_path

            # handle different types of links
            if new_site == "":
                new_site = cur_site
                new_url = urljoin(cur_url, new_path)

            # add new urls to queue
            if self.mode == "p":
                # add new url and site into dictionaries
                if new_site not in self.novelty:
                    self.novelty[new_site] = 1
                if new_url not in self.importance:
                    self.importance[new_url] = 1

                # update importance when link to other site
                if new_site != cur_site:
                    self.importance[new_url] += 1

                # we don't need to remove same urls from the queue since same urls' priority will only go up
                # and one url will only be visited once
                self.waiting_list.put((-(math.log(self.importance[new_url]) + 1/self.novelty[new_site]), new_url))
            else:
                self.waiting_list.put(new_url)

            cnt += 1

        return cnt, len(resp.content)

    # return True if we can keep crawling
    def crawl(self):
        # if we don't have url to crawl, stop crawling
        if self.waiting_list.empty():
            return False

        # try to get next not crawled url from the queue
        cur_url, cur_site, cur_priority = self.get_next()
        if cur_url is None:
            return True

        # don't visit a site too many times
        if cur_site in self.site_count:
            if self.site_count[cur_site] > 10:
                return True
            self.site_count[cur_site] += 1
        else:
            self.site_count[cur_site] = 1

        # try to download html from url
        resp = self.download_url(cur_url, cur_site, cur_priority)
        if resp is None:
            return True

        # parse the html downloaded
        num_of_new_links, length = self.parse_html(resp, cur_url, cur_site)

        # increase the number of total crawled urls
        self.number_of_urls += 1

        # increase the number of total links
        self.links += num_of_new_links

        # increase the total length of html files
        self.total_length += (length/8000)

        # log successful crawl
        logging.warning(threading.current_thread().name + ": " +
                        "priority = {}, code = 200, url = {}, size = {} bytes, crawled {} links".format(
                            round(cur_priority, 2),
                            cur_url,
                            length,
                            num_of_new_links)
                        )

        return True


def get_seeds(query):
    # pretend to be a user because google don't allow crawlers...
    user_agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.14; rv:65.0) Gecko/20100101 Firefox/65.0"
    query = query.replace(" ", "+")
    url = f"https://google.com/search?q={query}"

    headers = {"user-agent": user_agent}
    resp = requests.get(url, headers=headers)

    seeds = []

    if resp.status_code == 200:
        soup = BeautifulSoup(resp.content, "html.parser")
        for search in soup.find_all(id="search"):
            for g in search.find_all("div", class_="g"):
                for y in g.find_all("div", class_="yuRUbf"):
                    for link in y.find_all("a"):
                        seeds.append(link.get("href"))

    return seeds


def check_robot(site, url):
    robot_parser = RobotFileParser(site + "/robots.txt")

    try:
        ssl.match_hostname = lambda cert, hostname: True
        robot_parser.read()
    except error.URLError:
        return False
    except socket.timeout:
        return False
    except socket.error:
        return False
    except UnicodeError:
        return False
    except http.client.InvalidURL:
        return False
    except http.client.IncompleteRead:
        return False
    except http.client.BadStatusLine:
        return False
    except:
        return False

    if robot_parser.can_fetch("*", url):
        return True
    else:
        return False


class MyThread(threading.Thread):

    def __init__(self, my_crawler, goal):
        super().__init__()
        self.crawler = my_crawler
        self.goal = goal

    def run(self):
        while self.crawler.number_of_urls < self.goal:
            print(self.name)
            if not self.crawler.crawl():
                break
        print(self.name, "stopped")


if __name__ == "__main__":
    # set logging
    logging.basicConfig(filename="log.log", level=logging.WARNING, format="%(asctime)s - %(message)s")

    # set socket timeout
    socket.setdefaulttimeout(1)

    # init crawler
    seed_urls = get_seeds(input("Please input keywords: "))
    goals = input("Please set your goal of urls: ")
    num_of_thread = input("Please input number of threads: ")
    crawl_mode = input("Please input mode, 'p' for priority or 'r' for regular: ")
    crawler = Crawler(seed_urls, crawl_mode)

    start_time = time.time()

    # set multi threads
    threads = []
    for i in range(int(num_of_thread)):
        threads.append(MyThread(my_crawler=crawler, goal=int(goals)))
        threads[i].start()
        time.sleep(0.5)
    for i in range(int(num_of_thread)):
        threads[i].join()

    end_time = time.time()

    logging.warning("Tried to crawl for {} times, eventually crawled {} urls and found {} links".format(
        len(crawler.visited),
        crawler.number_of_urls,
        crawler.links)
    )
    logging.warning("Size of all crawled html files: {} MB".format(
        crawler.total_length/1000)
    )
    logging.warning("Time used: {}".format(end_time - start_time))
