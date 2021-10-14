#pragma once

// functions to handle all thread work for the app. Will required no external effort, simple call function plus 1-6
#include <iostream>
#include "QBXMLSync.h"

void runSyncThread(int i) {
    QBXMLSync qbs("QBX", "MEPBro Sampler", "TestPassword"); //tpass); // adding password coverage temporarily until we're ready for production values.

   // std::cout << "Start thread #" << i << '\n';

    switch (i) {
    case 1:
    //    std::cout << "Adding Inventory: \n";
        qbs.getInventory();
    //    std::cout << "Inventory Complete \n";
        break;
    case 2:
    //    std::cout << "Adding Sales Orders: \n";
        qbs.getSalesOrders();
    //    std::cout << "Sales orders Complete \n";
        break;
    case 3:
    //    std::cout << "Adding Estimates: \n";
        qbs.getEstimates();
    //    std::cout << "Estimates Complete \n";
        break;
    case 4:
    //    std::cout << "Adding Invoices: \n";
        qbs.getInvoices();
    //    std::cout << "Invoices Complete \n";
        break;
    case 5:
    //    std::cout << "Adding Customers: \n";
        qbs.getCustomers();
    //    std::cout << "Customers Complete \n";
        break;
    case 6:
    //    std::cout << "Adding Price Levels: \n";
        qbs.getPriceLevels();
    //    std::cout << "Price Levels Complete \n";

    //    std::cout << "Adding Sales Terms: \n";
        qbs.getSalesTerms();
    //    std::cout << "Sales Terms Complete \n";

    //    std::cout << "Adding Tax Codes: \n";
        qbs.getTaxCodes();
    //    std::cout << "Tax Codes Complete \n";

    //    std::cout << "Adding Sales Reps: \n";
        qbs.getSalesReps();
    //    std::cout << "Sales Reps Complete \n";
        break;
    }
    //std::cout << "Thread #" << i << " Complete \n";
}