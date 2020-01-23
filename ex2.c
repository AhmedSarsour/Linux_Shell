//Name: Ahmed Sarsour.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#define SIZE 255
//jobsProcess struct for the linked list of jobs.
struct jobsProcess{
    pid_t pid;
    char** procArgs;
    int argsSize;
    struct jobsProcess* next;
};
/**
 * removeFromList function: it gets the root of the linked list
 * and the process ID of the job we need to remove from the linked list,
 * and then removes it.
 * @param root of linked list.
 * @param jobPid the pid of process to remove from list.
 */
void removeFromList(struct jobsProcess* root, int jobPid) {
    struct jobsProcess* trace = root;
    //getting into the previous node before the node to be deleted.
    while(trace->next->pid != jobPid) {
        trace = trace->next;
    }
    //getting into the node we need to delete.
    struct jobsProcess* jobToDel = trace->next;
    //setting the previous's node->next into the next of the job to be deleted.
    trace->next = trace->next->next;
    //freeing the arguments of the job that need to be deleted.
    for (int i = 0;i < jobToDel->argsSize; i++) {
        free(jobToDel->procArgs[i]);
    }
    //freeing the args.
    free(jobToDel->procArgs);
    //freeing the node itself.
    free(jobToDel);
}
/**
 * freeItems function: it frees all the items that were allocated
 * during the program's work.
 * @param args of the commands.
 * @param root of the jobs linked list.
 * @param argsSize of the commands' args.
 */
void freeItems(char** args, struct jobsProcess* root, int argsSize) {
    //defining a node that will run to the end of the linked list.
    struct jobsProcess* toTheEnd = root->next;
    //moving on all the nodes of the linked list.
    while(toTheEnd != NULL) {
        //removing the nodes.
        removeFromList(root, toTheEnd->pid);
        toTheEnd = toTheEnd->next;
    }
    //lastly freeing the root of the linked list.
    free(root);
    //freeing the command arguments.
    int i;
    for(i = 0; i < argsSize; i++) {
        free(args[i]);
    }
    //freeing the args itself.
    free(args);
}
/**
 * executeJobs function: it gets access to the root of the linked
 * list and executes the "jobs" command.
 * @param root of the jobs' linked list.
 */
void executeJobs(struct jobsProcess* root) {
    struct jobsProcess* slideOver = root->next;
    pid_t pid;
    while(slideOver != NULL) {
        pid = slideOver->pid;
        //removing all the processes that finished their job.
        if(waitpid(pid, NULL, WNOHANG) == pid) {
            removeFromList(root, slideOver->pid);
        }
        slideOver = slideOver->next;
    }
    slideOver = root->next;
    int i;
    //printing the information of the jobs that are still
    //doing their job in the background.
    while(slideOver != NULL) {
        //printing the process ID of the process.
        printf("%d",slideOver->pid);
        //priting the information of the process.
        for(i = 0; i < slideOver->argsSize; i++) {
            printf(" %s", slideOver->procArgs[i]);
        }
        printf("\n");
        slideOver = slideOver->next;
    }
}
/**
 * cdCommand function: gets the arguments and executes
 * the cd command according to the given information.
 * @param argv of the commands.
 */
void cdCommand(char** argv) {
    //if the arguments are "cd" or "cd ~", then heading home.
    if (argv[1] == NULL || strcmp(argv[1], "~") == 0) {
        //entering the HOME directory.
        chdir(getenv("HOME"));
    }else {
        //entering the given directory, if it doesn't exist chdir return -1.
        if(chdir(argv[1]) == -1) {
            printf("no such directory: %s \n",argv[1]);
        }
    }
    return;
}

int main() {
    char* line;
    char** argv;
    char* token;
    int j;
    struct jobsProcess* root;
    //creating the root of the jobs linked list.
    root = (struct jobsProcess*) malloc(sizeof(struct jobsProcess));
    root->next = NULL;
    struct jobsProcess* jobs = root;
    do {
        printf("prompt> ");
        //allocating 1024 bytes for the user arguments.
        line = (char*)malloc(1024* sizeof(char));
        //if line is NULL means that the allocation failed.
        if (line == NULL) {
            printf("allocating memory had failed.\n");
            exit(1);
        }
        //getting the line from the user.
        fgets(line, SIZE, stdin);
        //getting the length of the given input.
        int length = strlen(line);
        j = 0;
        //couting the number of the words + 1.
        int i = 0, counter = 2;
        for (i = 0; i < length; i++) {
            if (line[i] == ' ') {
                counter++;
            }
        }
        //delimiting the line by the spaces.
        token = strtok(line, " ");
        //allocating enough memory according to the number of words + 1 for null.
        argv = (char**)malloc(counter * sizeof(char*));
        //saving the words in the argv variable.
        while(token !=  NULL) {
            //allocating sufficient space for each word.
            argv[j] = (char*) malloc(strlen(token) * sizeof(char));
            if (argv[j] == NULL) {
                printf("allocating memory had failed.\n");
                exit(1);
            }
            //copying the token in the argument.
            strcpy(argv[j], token);
            j++;
            //moving into the next word.
            token = strtok(NULL, " ");
        }
        //removing the "\n" from the last word.
        argv[j-1] = strtok(argv[j - 1],"\n");
        //setting the last spot of the arguments to NULL.
        argv[j] = NULL;
        //freeing the input.
        free(line);
        //if nothing was entered, then continuing into next loop.
        if(argv[0] == NULL) {
            continue;
        }
        //if the command was exit then freeing the items then exiting.
        if (strcmp(argv[0],"exit") == 0 ){
            freeItems(argv, root, counter);
            return 0;
            //if the command was cd then executing the cd command.
        }else if (strcmp(argv[0], "cd") == 0) {
            cdCommand(argv);
            continue;
            //if the command was jobs then executing the jobs command.
        }else if(strcmp(argv[0],"jobs") == 0) {
            executeJobs(root);
            continue;
        }
        //producing a child process.
        pid_t pid = fork();
        //if pid == 0 means we are in the child process.
        if (pid == 0) {
            //if the command had "&" at the end then we put NULL in argv.
            if (strcmp(argv[j - 1], "&") == 0) {
                argv[j - 1] = NULL;
            }
            //printing the pid of the child.
            printf("%d\n", getpid());
            //executing the command.
            int status = execvp(argv[0], argv);
            fprintf(stderr, "Error in system call\n");
            return 0;
            //else, we are in the parent process.
        }else {
            //if there is no "&" in the command, the parent waits for child.
            if (strcmp(argv[j - 1],"&") != 0) {
                while (wait(NULL) != -1 || errno != ECHILD) {
                    // nothing, just waiting for all the children to finish.
                }
                //else we have a "&" in the command.
            } else {
                //adding the current process to the jobs list.
                jobs = root;
                while (jobs->next != NULL) {
                    jobs = jobs->next;
                }
                //creating a new node for the process and filling the informaton.
                jobs->next = (struct jobsProcess *) malloc(sizeof(struct jobsProcess));
                jobs = jobs->next;
                jobs->pid = pid;
                jobs->procArgs = (char **) malloc((counter - 2) * sizeof(char *));
                jobs->argsSize = counter - 2;
                jobs->next = NULL;
                //copying the arguments into the list's node.
                for (i = 0; i < counter - 2; i++) {
                    jobs->procArgs[i] = (char *) malloc(strlen(argv[i]));
                    if (jobs->procArgs[i] == NULL) {
                        printf("allocating memory had failed.\n");
                        exit(1);
                    }
                    strcpy(jobs->procArgs[i], argv[i]);
                }
            }
        }
        //lastly we free the command args..
        for (i = 0;i < counter; i++) {
            free(argv[i]);
        }
        free(argv);
        //the code runs forever until the user enters "exit".
    }while(1);
}
