#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CUSTOMERS 200
#define MAX_PACKAGES  200
#define ADMIN_USER "jabid"
#define ADMIN_PASS "jabid1234"
#define CUSTOMER_FILE "customers.dat"
#define PACKAGE_FILE  "packages.dat"

/* ====================================
   STEP 1: DEFINE OUR DATA STRUCTURES
   ====================================*/
struct TrackNode {
    char status[40];
    struct TrackNode *next;
};
typedef struct TrackNode TrackNode;

struct Customer {
    int   id;
    char  name[50];
    char  password[20];
    char  phone[15];
    int   productsBought;
    float totalExpense;
};
typedef struct Customer Customer;

struct Package {
    int   trackId;
    int   customerId;
    char  productName[50];
    float amount;
    int   isPaid;                
    char  currentStatus[40];
    TrackNode *history;          
};
typedef struct Package Package;

struct BSTNode {
    int trackId;
    int packageIndex;           
    struct BSTNode *left;
    struct BSTNode *right;
};
typedef struct BSTNode BSTNode;

/* =============================
   STEP 2: GLOBAL VARIABLES
   =============================*/

Customer customers[MAX_CUSTOMERS];  
int customerCount = 0;               

Package packages[MAX_PACKAGES];      
int packageCount = 0;

float totalRevenue = 0;

int queueItems[MAX_PACKAGES];
int queueFront = 0;
int queueSize = 0;

int  stackPackageIndex[MAX_PACKAGES];
char stackOldStatus[MAX_PACKAGES][40];
int  stackTop = -1;

BSTNode *bstRoot = NULL;

/* ==============================
   STEP 3: FUNCTION PROTOTYPES
   ==============================*/
void loadData(void);
void saveData(void);

void showMainMenu(void);
void adminLogin(void);
void showAdminMenu(void);

void customerRegister(void);
void customerLogin(void);
void showCustomerMenu(int customerIndex);

void addPackage(void);
void makePayment(int customerIndex);
void trackPackage(void);
void updateStatus(void);

void viewRevenue(void);
void viewAllCustomers(void);
void viewAllPackages(void);

void addTrackNode(Package *package, const char *status);
void freeOneHistory(TrackNode *node);
void freeAllHistories(void);

int findCustomerIndexById(int id);
int findPackageIndexByTrackId(int trackId);
void searchPackageMenu(void);

void enqueuePackage(int trackId);
int  dequeuePackage(void);
int  isQueueEmpty(void);
void processNextDispatch(void);

void pushUndo(int packageIndex, const char *oldStatus);
int  popUndo(int *packageIndexOut, char *oldStatusOut);
void undoLastStatusUpdate(void);

BSTNode* bstInsert(BSTNode *root, int trackId, int packageIndex);
void bstPrintSorted(BSTNode *root);
void bstFree(BSTNode *root);

void clearInputBuffer(void);

/* ===============================
   MAIN - the program starts here
   ===============================*/
int main(void) {
    loadData();          
    showMainMenu();       
    saveData();           
    freeAllHistories();   
    bstFree(bstRoot);
    printf("\nData saved. Goodbye!\n");
    return 0;
}

/* =========================
   SMALL HELPER FUNCTIONS
   =========================*/

void clearInputBuffer(void) {
    int leftoverCharacter;
    while (1) {
        leftoverCharacter = getchar();
        if (leftoverCharacter == '\n' || leftoverCharacter == EOF) {
            break;
        }
    }
}

int findCustomerIndexById(int id) {
    for (int i = 0; i < customerCount; i++) {
        if (customers[i].id == id) {
            return i;
        }
    }
    return -1;
}
int findPackageIndexByTrackId(int trackId) {
    for (int i = 0; i < packageCount; i++) {
        if (packages[i].trackId == trackId) {
            return i;
        }
    }
    return -1;
}

/* ================================
   STEP 4: LINKED LIST FUNCTIONS
   ================================*/

void addTrackNode(Package *package, const char *status) {
  
    TrackNode *newNode = malloc(sizeof(TrackNode));

    strcpy(newNode->status, status);

    newNode->next = package->history;

    package->history = newNode;

    strcpy(package->currentStatus, status);
}

void freeOneHistory(TrackNode *node) {
    while (node != NULL) {
        TrackNode *nextNode = node->next;  
        free(node);                       
        node = nextNode;                   
    }
}
void freeAllHistories(void) {
    for (int i = 0; i < packageCount; i++) {
        freeOneHistory(packages[i].history);
    }
}

/* ========================
   STEP 5: BINARY SEARCH
   ========================*/
int binarySearchCustomer(int id) {
    int low = 0;
    int high = customerCount - 1;

    while (low <= high) {
        int middle = (low + high) / 2;

        if (customers[middle].id == id) {
            return middle;                
        }
        if (customers[middle].id < id) {
            low = middle + 1;              
        } else {
            high = middle - 1;            
        }
    }
    return -1;
}

int binarySearchPackage(int trackId) {
    int low = 0;
    int high = packageCount - 1;

    while (low <= high) {
        int middle = (low + high) / 2;

        if (packages[middle].trackId == trackId) {
            return middle;
        }
        if (packages[middle].trackId < trackId) {
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }
    return -1;
}

void searchPackageMenu(void) {
    int trackId;
    printf("\nEnter Tracking ID to search: ");
    scanf("%d", &trackId);
    clearInputBuffer();

    int foundIndex = binarySearchPackage(trackId);
    if (foundIndex == -1) {
        printf("Package not found.\n");
        return;
    }

    printf("FOUND -> Product: %s | Amount: %.2f | Status: %s\n",
           packages[foundIndex].productName,
           packages[foundIndex].amount,
           packages[foundIndex].currentStatus);
}

/* ====================
   STEP 6: QUEUE 
   ====================*/

void enqueuePackage(int trackId) {
    if (queueSize == MAX_PACKAGES) {
        printf("Dispatch queue is full!\n");
        return;
    }
    int nextEmptySlot = queueFront + queueSize;
    queueItems[nextEmptySlot] = trackId;
    queueSize = queueSize + 1;
}

int dequeuePackage(void) {
    if (queueSize == 0) {
        return -1; 
    }
    int trackId = queueItems[queueFront];
    queueFront = queueFront + 1; 
    queueSize = queueSize - 1;
    return trackId;
}

int isQueueEmpty(void) {
    if (queueSize == 0) {
        return 1; 
    }
    return 0; 
}

void processNextDispatch(void) {
    if (isQueueEmpty()) {
        printf("Dispatch queue is empty -- nothing waiting to go out.\n");
        return;
    }

    int trackId = dequeuePackage();
    int packageIndex = binarySearchPackage(trackId);
    if (packageIndex == -1) {
        return; 
    }

    pushUndo(packageIndex, packages[packageIndex].currentStatus);
    addTrackNode(&packages[packageIndex], "Picked Up");

    printf("Dispatched Tracking ID %d -> status is now 'Picked Up'\n", trackId);
    printf("Packages still waiting in queue: %d\n", queueSize);
}

/* ========================
   STEP 7: STACK 
   ========================*/

void pushUndo(int packageIndex, const char *oldStatus) {
    if (stackTop == MAX_PACKAGES - 1) {
        return; 
    }
    stackTop = stackTop + 1;
    stackPackageIndex[stackTop] = packageIndex;
    strcpy(stackOldStatus[stackTop], oldStatus);
}

int popUndo(int *packageIndexOut, char *oldStatusOut) {
    if (stackTop == -1) {
        return 0; 
    }
    *packageIndexOut = stackPackageIndex[stackTop];
    strcpy(oldStatusOut, stackOldStatus[stackTop]);
    stackTop = stackTop - 1;
    return 1;
}

void undoLastStatusUpdate(void) {
    int packageIndex;
    char oldStatus[40];

    int somethingToUndo = popUndo(&packageIndex, oldStatus);
    if (somethingToUndo == 0) {
        printf("Nothing to undo.\n");
        return;
    }

    Package *package = &packages[packageIndex];

    TrackNode *mostRecentNode = package->history;
    if (mostRecentNode != NULL) {
        package->history = mostRecentNode->next;
        free(mostRecentNode);
    }
    strcpy(package->currentStatus, oldStatus);

    printf("Tracking ID %d reverted back to status '%s'\n", package->trackId, package->currentStatus);
}

/* =============================
   STEP 8: BINARY SEARCH TREE
   =============================*/

BSTNode* bstInsert(BSTNode *root, int trackId, int packageIndex) {
    if (root == NULL) {
        BSTNode *newNode = malloc(sizeof(BSTNode));
        newNode->trackId = trackId;
        newNode->packageIndex = packageIndex;
        newNode->left = NULL;
        newNode->right = NULL;
        return newNode;
    }

    if (trackId < root->trackId) {
        root->left = bstInsert(root->left, trackId, packageIndex);
    } else if (trackId > root->trackId) {
        root->right = bstInsert(root->right, trackId, packageIndex);
    }
    return root;
}

void bstPrintSorted(BSTNode *root) {
    if (root == NULL) {
        return; 
    }

    bstPrintSorted(root->left);

    Package *package = &packages[root->packageIndex];
    printf("%-8d %-10d %-15s %-10.2f %-8s %-20s\n",
           package->trackId, package->customerId, package->productName,
           package->amount, package->isPaid ? "Yes" : "No", package->currentStatus);

    bstPrintSorted(root->right);
}

void bstFree(BSTNode *root) {
    if (root == NULL) {
        return;
    }
    bstFree(root->left);
    bstFree(root->right);
    free(root);
}

/* ====================================
   STEP 9: SAVING / LOADING FROM DISK
   ====================================*/
void loadData(void) {
    FILE *customerFile = fopen(CUSTOMER_FILE, "rb");
    if (customerFile != NULL) {
        fread(&customerCount, sizeof(int), 1, customerFile);
        fread(customers, sizeof(Customer), customerCount, customerFile);
        fclose(customerFile);
    }

    FILE *packageFile = fopen(PACKAGE_FILE, "rb");
    if (packageFile != NULL) {
        fread(&packageCount, sizeof(int), 1, packageFile);
        fread(packages, sizeof(Package), packageCount, packageFile);
        fclose(packageFile);

        totalRevenue = 0;
        for (int i = 0; i < packageCount; i++) {
            packages[i].history = NULL;
            addTrackNode(&packages[i], packages[i].currentStatus); 

            if (packages[i].isPaid) {
                totalRevenue = totalRevenue + packages[i].amount;
            }
            bstRoot = bstInsert(bstRoot, packages[i].trackId, i);
        }
    }
   
}

void saveData(void) {
    FILE *customerFile = fopen(CUSTOMER_FILE, "wb");
    if (customerFile != NULL) {
        fwrite(&customerCount, sizeof(int), 1, customerFile);
        fwrite(customers, sizeof(Customer), customerCount, customerFile);
        fclose(customerFile);
    }

    FILE *packageFile = fopen(PACKAGE_FILE, "wb");
    if (packageFile != NULL) {
        fwrite(&packageCount, sizeof(int), 1, packageFile);
        fwrite(packages, sizeof(Package), packageCount, packageFile);
        fclose(packageFile);
    }
}

/* ==========================
   STEP 10: MAIN MENU
   ==========================*/
void showMainMenu(void) {
    int choice;
    do {
        printf("\n========== COURIER MANAGEMENT SYSTEM ==========\n");
        printf("1. Admin Login\n");
        printf("2. Customer Login\n");
        printf("3. Customer Register\n");
        printf("4. Exit\n");
        printf("Enter choice: ");

        int result = scanf("%d", &choice);
        if (result == EOF) {
            choice = 4;
            break;
        }
        if (result != 1) {
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        if (choice == 1) {
            adminLogin();
        } else if (choice == 2) {
            customerLogin();
        } else if (choice == 3) {
            customerRegister();
        } else if (choice == 4) {
            printf("Exiting...\n");
        } else {
            printf("Invalid choice!\n");
        }
    } while (choice != 4);
}

/* ==============================
   STEP 11: ADMIN
   ==============================*/
void adminLogin(void) {
    char username[20];
    char password[20];

    printf("\n--- ADMIN LOGIN ---\n");
    printf("Username: ");
    scanf("%19s", username);
    printf("Password: ");
    scanf("%19s", password);
    clearInputBuffer();

    if (strcmp(username, ADMIN_USER) == 0 && strcmp(password, ADMIN_PASS) == 0) {
        printf("Login successful! Welcome Admin.\n");
        showAdminMenu();
    } else {
        printf("Invalid admin credentials!\n");
    }
}

void showAdminMenu(void) {
    int choice;
    do {
        printf("\n---------- ADMIN MENU ----------\n");
        printf("1. View All Customers\n");
        printf("2. View All Packages (sorted, via Binary Search Tree)\n");
        printf("3. Add New Package\n");
        printf("4. Update Package Status\n");
        printf("5. Undo Last Status Update (Stack)\n");
        printf("6. Process Next Dispatch (Queue)\n");
        printf("7. Search Package by Tracking ID (Binary Search)\n");
        printf("8. View Total Revenue\n");
        printf("9. Logout\n");
        printf("Enter choice: ");

        int result = scanf("%d", &choice);
        if (result == EOF) {
            choice = 9;
            break;
        }
        if (result != 1) {
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        if (choice == 1) {
            viewAllCustomers();
        } else if (choice == 2) {
            viewAllPackages();
        } else if (choice == 3) {
            addPackage();
        } else if (choice == 4) {
            updateStatus();
        } else if (choice == 5) {
            undoLastStatusUpdate();
        } else if (choice == 6) {
            processNextDispatch();
        } else if (choice == 7) {
            searchPackageMenu();
        } else if (choice == 8) {
            viewRevenue();
        } else if (choice == 9) {
            printf("Logging out of admin...\n");
        } else {
            printf("Invalid choice!\n");
        }
    } while (choice != 9);
}

/* ====================================
   STEP 12: CUSTOMER LOGIN / REGISTER
   ====================================*/
void customerRegister(void) {
    if (customerCount >= MAX_CUSTOMERS) {
        printf("Customer limit reached!\n");
        return;
    }

    Customer newCustomer;
    newCustomer.id = 1000 + customerCount + 1; 

    printf("\n--- CUSTOMER REGISTRATION ---\n");
    printf("Enter your name (no spaces): ");
    scanf("%49s", newCustomer.name);
    printf("Set a password: ");
    scanf("%19s", newCustomer.password);
    printf("Enter phone number: ");
    scanf("%14s", newCustomer.phone);
    clearInputBuffer();

    newCustomer.productsBought = 0;
    newCustomer.totalExpense = 0;

    customers[customerCount] = newCustomer;
    customerCount = customerCount + 1;

    printf("Registration successful! Your Customer ID is %d\n", newCustomer.id);
    printf("Please remember this ID, it is required to login.\n");
}

void customerLogin(void) {
    int id;
    char password[20];

    printf("\n--- CUSTOMER LOGIN ---\n");
    printf("Enter Customer ID: ");
    scanf("%d", &id);
    printf("Enter Password: ");
    scanf("%19s", password);
    clearInputBuffer();

    int customerIndex = binarySearchCustomer(id);
    if (customerIndex == -1) {
        printf("Invalid ID or password!\n");
        return;
    }
    if (strcmp(customers[customerIndex].password, password) != 0) {
        printf("Invalid ID or password!\n");
        return;
    }

    printf("Login successful! Welcome %s.\n", customers[customerIndex].name);
    showCustomerMenu(customerIndex);
}

void showCustomerMenu(int customerIndex) {
    int choice;
    do {
        printf("\n---------- CUSTOMER MENU (%s) ----------\n", customers[customerIndex].name);
        printf("1. View My Info\n");
        printf("2. Track My Packages\n");
        printf("3. Make Payment\n");
        printf("4. Logout\n");
        printf("Enter choice: ");

        int result = scanf("%d", &choice);
        if (result == EOF) {
            choice = 4;
            break;
        }
        if (result != 1) {
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        if (choice == 1) {
            Customer *c = &customers[customerIndex];
            printf("\nID: %d\n", c->id);
            printf("Name: %s\n", c->name);
            printf("Phone: %s\n", c->phone);
            printf("Products Bought: %d\n", c->productsBought);
            printf("Total Expense: %.2f\n", c->totalExpense);
        } else if (choice == 2) {
            trackPackage();
        } else if (choice == 3) {
            makePayment(customerIndex);
        } else if (choice == 4) {
            printf("Logging out...\n");
        } else {
            printf("Invalid choice!\n");
        }
    } while (choice != 4);
}

/* ===========================
   STEP 13: PACKAGES / TRACKING
   ===========================*/
void addPackage(void) {
    if (packageCount >= MAX_PACKAGES) {
        printf("Package limit reached!\n");
        return;
    }

    int customerId;
    printf("\n--- ADD NEW PACKAGE ---\n");
    printf("Enter Customer ID: ");
    scanf("%d", &customerId);
    clearInputBuffer();

    int customerIndex = binarySearchCustomer(customerId);
    if (customerIndex == -1) {
        printf("Customer not found!\n");
        return;
    }

    Package newPackage;
    newPackage.trackId = 2000 + packageCount + 1; 
    newPackage.customerId = customerId;
    newPackage.isPaid = 0;
    newPackage.history = NULL;

    printf("Enter product name (no spaces): ");
    scanf("%49s", newPackage.productName);
    printf("Enter amount (price) for this package: ");
    scanf("%f", &newPackage.amount);
    clearInputBuffer();

    addTrackNode(&newPackage, "Order Placed");

    int newPackageIndex = packageCount;
    packages[newPackageIndex] = newPackage;
    packageCount = packageCount + 1;

    bstRoot = bstInsert(bstRoot, newPackage.trackId, newPackageIndex);
    enqueuePackage(newPackage.trackId);
    customers[customerIndex].productsBought = customers[customerIndex].productsBought + 1;

    printf("Package added successfully! Tracking ID: %d\n", newPackage.trackId);
    printf("It has joined the dispatch queue (%d package(s) now waiting).\n", queueSize);
}

void updateStatus(void) {
    int trackId;
    printf("\n--- UPDATE PACKAGE STATUS ---\n");
    printf("Enter Tracking ID: ");
    scanf("%d", &trackId);
    clearInputBuffer();

    int packageIndex = binarySearchPackage(trackId);
    if (packageIndex == -1) {
        printf("Package not found!\n");
        return;
    }

    printf("Choose new status:\n");
    printf("1. Picked Up\n2. In Transit\n3. Out for Delivery\n4. Delivered\n");
    int choice;
    scanf("%d", &choice);
    clearInputBuffer();

    const char *newStatus;
    if (choice == 1) {
        newStatus = "Picked Up";
    } else if (choice == 2) {
        newStatus = "In Transit";
    } else if (choice == 3) {
        newStatus = "Out for Delivery";
    } else if (choice == 4) {
        newStatus = "Delivered";
    } else {
        printf("Invalid option!\n");
        return;
    }

    pushUndo(packageIndex, packages[packageIndex].currentStatus); 
    addTrackNode(&packages[packageIndex], newStatus);

    printf("Status updated to '%s' for Tracking ID %d\n", newStatus, trackId);
    printf("(This change can be undone from the admin menu.)\n");
}

void trackPackage(void) {
    int trackId;
    printf("\nEnter Tracking ID: ");
    scanf("%d", &trackId);
    clearInputBuffer();

    int packageIndex = binarySearchPackage(trackId);
    if (packageIndex == -1) {
        printf("Package not found!\n");
        return;
    }

    Package *p = &packages[packageIndex];
    printf("\nTracking ID   : %d\n", p->trackId);
    printf("Product       : %s\n", p->productName);
    printf("Amount        : %.2f\n", p->amount);
    printf("Payment       : %s\n", p->isPaid ? "Paid" : "Unpaid");
    printf("Current Status: %s\n", p->currentStatus);

    printf("\nFull Tracking History (latest first):\n");
    int step = 1;
    TrackNode *node = p->history;
    while (node != NULL) {
        printf("  %d. %s\n", step, node->status);
        step = step + 1;
        node = node->next;
    }
}

/* =============================
   STEP 14: PAYMENT
   =============================*/
void makePayment(int customerIndex) {
    int customerId = customers[customerIndex].id;

    printf("\n--- YOUR UNPAID PACKAGES ---\n");
    int foundAny = 0;
    for (int i = 0; i < packageCount; i++) {
        if (packages[i].customerId == customerId && packages[i].isPaid == 0) {
            printf("TrackID: %d | Product: %s | Amount: %.2f\n",
                   packages[i].trackId, packages[i].productName, packages[i].amount);
            foundAny = 1;
        }
    }
    if (foundAny == 0) {
        printf("You have no pending payments.\n");
        return;
    }

    int trackId;
    printf("Enter Tracking ID to pay for: ");
    scanf("%d", &trackId);
    clearInputBuffer();

    int packageIndex = binarySearchPackage(trackId);
    if (packageIndex == -1 || packages[packageIndex].customerId != customerId) {
        printf("Invalid Tracking ID!\n");
        return;
    }
    if (packages[packageIndex].isPaid == 1) {
        printf("This package is already paid.\n");
        return;
    }

    packages[packageIndex].isPaid = 1;
    totalRevenue = totalRevenue + packages[packageIndex].amount;
    customers[customerIndex].totalExpense = customers[customerIndex].totalExpense + packages[packageIndex].amount;
    addTrackNode(&packages[packageIndex], "Payment Received");

    printf("Payment of %.2f successful for Tracking ID %d!\n", packages[packageIndex].amount, trackId);
}

/* ===============================
   STEP 15: REPORTS
   ===============================*/
void viewRevenue(void) {
    int paidCount = 0;
    int unpaidCount = 0;
    float pendingAmount = 0;

    for (int i = 0; i < packageCount; i++) {
        if (packages[i].isPaid == 1) {
            paidCount = paidCount + 1;
        } else {
            unpaidCount = unpaidCount + 1;
            pendingAmount = pendingAmount + packages[i].amount;
        }
    }

    printf("\n===== REVENUE REPORT =====\n");
    printf("Total Revenue Collected: %.2f\n", totalRevenue);
    printf("Paid Packages   : %d\n", paidCount);
    printf("Unpaid Packages : %d\n", unpaidCount);
    printf("Pending Amount  : %.2f\n", pendingAmount);
}

void viewAllCustomers(void) {
    printf("\n===== CUSTOMER INFO =====\n");
    printf("%-6s %-15s %-12s %-15s %-10s\n", "ID", "Name", "Products", "Expense", "Phone");

    for (int i = 0; i < customerCount; i++) {
        printf("%-6d %-15s %-12d %-15.2f %-10s\n",
               customers[i].id, customers[i].name,
               customers[i].productsBought, customers[i].totalExpense,
               customers[i].phone);
    }
}

void viewAllPackages(void) {
    printf("\n===== ALL PACKAGES (sorted by Tracking ID, via BST) =====\n");
    printf("%-8s %-10s %-15s %-10s %-8s %-20s\n",
           "TrackID", "CustID", "Product", "Amount", "Paid", "Status");

    if (bstRoot == NULL) {
        printf("(no packages yet)\n");
        return;
    }
    bstPrintSorted(bstRoot);
}