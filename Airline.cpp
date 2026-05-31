#include "../include/Airline.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <iostream>

using namespace std;

// Constructor
Airline::Airline(string name) : airlineName(name), nextTicketID(1) {}

// Destructor - shared_ptr handles memory cleanup automatically
Airline::~Airline() {}

// Generate a unique ticket ID like TKT001, TKT002, etc.
string Airline::generateTicketID() {
    string id = "TKT";
    if (nextTicketID < 10) id += "00";
    else if (nextTicketID < 100) id += "0";
    id += to_string(nextTicketID);
    nextTicketID++;
    return id;
}

// ===================== FLIGHT MANAGEMENT =====================

void Airline::addFlight(shared_ptr<Flight> flight) {
    // Check if flight number already exists
    for (const auto& f : flights) {
        if (f->getFlightNumber() == flight->getFlightNumber()) {
            cout << "Error: Flight " << flight->getFlightNumber() << " already exists!" << endl;
            return;
        }
    }
    flights.push_back(flight);
    revenueByFlight[flight->getFlightNumber()] = 0.0;
    cout << "Flight " << flight->getFlightNumber() << " added successfully!" << endl;
}

bool Airline::removeFlight(const string& flightNumber) {
    // Check if any active tickets exist for this flight
    for (const auto& ticket : tickets) {
        if (ticket.getFlightNumber() == flightNumber &&
            ticket.getBookingStatus() == "Booked") {
            cout << "Cannot remove flight " << flightNumber
                 << ". Active bookings exist!" << endl;
            return false;
        }
    }

    // Find and remove the flight
    auto it = find_if(flights.begin(), flights.end(),
        [&flightNumber](const shared_ptr<Flight>& f) {
            return f->getFlightNumber() == flightNumber;
        });

    if (it != flights.end()) {
        flights.erase(it);
        cout << "Flight " << flightNumber << " removed successfully!" << endl;
        return true;
    }

    cout << "Flight " << flightNumber << " not found!" << endl;
    return false;
}

// Search flights by flight number (using template search utility)
vector<shared_ptr<Flight>> Airline::searchFlightsByNumber(const string& flightNum) {
    vector<shared_ptr<Flight>> results;
    for (const auto& f : flights) {
        if (f->getFlightNumber() == flightNum) {
            results.push_back(f);
        }
    }
    return results;
}

// Search flights by route
vector<shared_ptr<Flight>> Airline::searchFlightsByRoute(const string& origin, const string& dest) {
    vector<shared_ptr<Flight>> results;
    for (const auto& f : flights) {
        if (f->getOrigin() == origin && f->getDestination() == dest) {
            results.push_back(f);
        }
    }
    return results;
}

// Search flights by date
vector<shared_ptr<Flight>> Airline::searchFlightsByDate(const string& date) {
    vector<shared_ptr<Flight>> results;
    for (const auto& f : flights) {
        if (f->getDepartureDate() == date) {
            results.push_back(f);
        }
    }
    return results;
}

// List all flights
void Airline::listAllFlights() const {
    if (flights.empty()) {
        cout << "No flights available." << endl;
        return;
    }
    cout << "\n========== ALL FLIGHTS ==========" << endl;
    for (const auto& f : flights) {
        f->displayDetails();
        cout << endl;
    }
}

// ===================== PASSENGER MANAGEMENT =====================

void Airline::registerPassenger(shared_ptr<Passenger> passenger) {
    // Check if passenger ID already exists
    for (const auto& p : passengers) {
        if (p->getPassengerID() == passenger->getPassengerID()) {
            cout << "Error: Passenger " << passenger->getPassengerID()
                 << " already registered!" << endl;
            return;
        }
    }
    passengers.push_back(passenger);
    cout << "Passenger " << passenger->getName() << " registered successfully!" << endl;
}

bool Airline::removePassenger(const string& passengerID) {
    // Check for active bookings
    for (const auto& ticket : tickets) {
        if (ticket.getPassengerID() == passengerID &&
            ticket.getBookingStatus() == "Booked") {
            cout << "Cannot remove passenger. Active bookings exist!" << endl;
            return false;
        }
    }

    auto it = find_if(passengers.begin(), passengers.end(),
        [&passengerID](const shared_ptr<Passenger>& p) {
            return p->getPassengerID() == passengerID;
        });

    if (it != passengers.end()) {
        passengers.erase(it);
        cout << "Passenger " << passengerID << " removed successfully!" << endl;
        return true;
    }

    cout << "Passenger " << passengerID << " not found!" << endl;
    return false;
}

void Airline::viewBookingHistory(const string& passengerID) const {
    cout << "\n=== Booking History for Passenger " << passengerID << " ===" << endl;
    bool found = false;
    for (const auto& ticket : tickets) {
        if (ticket.getPassengerID() == passengerID) {
            cout << ticket;
            found = true;
        }
    }
    if (!found) {
        cout << "No bookings found for this passenger." << endl;
    }
}

shared_ptr<Passenger> Airline::findPassenger(const string& passengerID) {
    for (const auto& p : passengers) {
        if (p->getPassengerID() == passengerID) {
            return p;
        }
    }
    return nullptr;
}

// ===================== BOOKING =====================

void Airline::bookTicket(const string& passengerID, const string& flightNumber, const string& currentDate) {
    // Find the passenger
    shared_ptr<Passenger> passenger = findPassenger(passengerID);
    if (!passenger) {
        cout << "Passenger " << passengerID << " not found! Please register first." << endl;
        return;
    }

    // Find the flight
    shared_ptr<Flight> flight = nullptr;
    for (const auto& f : flights) {
        if (f->getFlightNumber() == flightNumber) {
            flight = f;
            break;
        }
    }
    if (!flight) {
        cout << "Flight " << flightNumber << " not found!" << endl;
        return;
    }

    // Check for duplicate booking - throw exception if already booked
    for (const auto& ticket : tickets) {
        if (ticket.getPassengerID() == passengerID &&
            ticket.getFlightNumber() == flightNumber &&
            ticket.getBookingStatus() == "Booked") {
            throw DuplicateBookingException(passengerID, flightNumber);
        }
    }

    // Check if flight is full - throw exception
    if (flight->getAvailableSeats() <= 0) {
        throw FlightFullException(flightNumber);
    }

    // Calculate fare based on flight type
    double fare = flight->calculateBaseFare();

    // Book the seat
    flight->bookSeat();

    // Generate seat number (simple: row + letter)
    int seatRow = flight->getTotalSeats() - flight->getAvailableSeats();
    string seatNumber = to_string(seatRow) + "A";

    // Create ticket
    string ticketID = generateTicketID();
    Ticket newTicket(ticketID, passengerID, flightNumber, seatNumber,
                     fare, "Booked", currentDate);

    tickets.push_back(newTicket);

    // Update revenue
    revenueByFlight[flightNumber] += fare;

    // Add loyalty points to passenger
    int points = (int)(fare * passenger->getLoyaltyMultiplier());
    passenger->addLoyaltyPoints(points);

    cout << "\nBooking Successful!" << endl;
    cout << newTicket;
}

// ===================== CANCELLATION & REFUND =====================

void Airline::cancelTicket(const string& ticketID, const string& currentDate) {
    // Find the ticket
    Ticket* foundTicket = nullptr;
    for (auto& ticket : tickets) {
        if (ticket.getTicketID() == ticketID) {
            foundTicket = &ticket;
            break;
        }
    }

    if (!foundTicket) {
        throw InvalidCancellationException("Ticket " + ticketID + " not found!");
    }

    if (foundTicket->getBookingStatus() == "Cancelled") {
        throw InvalidCancellationException("Ticket " + ticketID + " is already cancelled!");
    }

    // Find the passenger to get refund percentage (polymorphic call)
    shared_ptr<Passenger> passenger = findPassenger(foundTicket->getPassengerID());
    if (!passenger) {
        throw InvalidCancellationException("Passenger not found for this ticket!");
    }

    // Calculate refund based on passenger type (polymorphism)
    double refundPercent = passenger->getRefundPercentage();

    // Reduce refund if cancellation is close to departure
    // Simple rule: if same date as departure, only 50% of normal refund
    // Find the flight to check departure date
    for (const auto& f : flights) {
        if (f->getFlightNumber() == foundTicket->getFlightNumber()) {
            if (f->getDepartureDate() == currentDate) {
                refundPercent *= 0.5; // halve the refund if same day
            }
            // Give back the seat
            f->cancelSeat();
            break;
        }
    }

    double refundAmount = foundTicket->getFarePaid() * (refundPercent / 100.0);

    // Update ticket status
    foundTicket->setBookingStatus("Cancelled");

    // Update revenue
    revenueByFlight[foundTicket->getFlightNumber()] -= refundAmount;

    cout << "\nCancellation Successful!" << endl;
    cout << "Ticket ID    : " << ticketID << endl;
    cout << "Refund Rate  : " << refundPercent << "%" << endl;
    cout << "Refund Amount: $" << fixed << setprecision(2) << refundAmount << endl;
}

// ===================== REPORTS =====================

// Show flights departing today
void Airline::todaysDepartures(const string& today) const {
    cout << "\n========== TODAY'S DEPARTURES (" << today << ") ==========" << endl;
    bool found = false;
    for (const auto& f : flights) {
        if (f->getDepartureDate() == today) {
            f->displayDetails();
            found = true;
        }
    }
    if (!found) {
        cout << "No flights departing today." << endl;
    }
}

// Show occupancy for each flight
void Airline::occupancyReport() const {
    cout << "\n========== OCCUPANCY REPORT ==========" << endl;
    cout << left << setw(12) << "Flight"
         << setw(15) << "Route"
         << setw(12) << "Date"
         << setw(10) << "Seats"
         << "Occupancy" << endl;
    cout << string(60, '-') << endl;

    for (const auto& f : flights) {
        cout << left << setw(12) << f->getFlightNumber()
             << setw(15) << (f->getOrigin() + "->" + f->getDestination())
             << setw(12) << f->getDepartureDate()
             << setw(10) << (to_string(f->getTotalSeats() - f->getAvailableSeats()) +
                             "/" + to_string(f->getTotalSeats()))
             << fixed << setprecision(1) << f->getOccupancyPercentage() << "%"
             << endl;
    }
}

// Show top 5 highest-revenue flights
void Airline::topRevenueFlights() const {
    cout << "\n========== TOP 5 REVENUE FLIGHTS ==========" << endl;

    // Copy revenue map to a vector for sorting (using STL sort)
    vector<pair<string, double>> revenueList(revenueByFlight.begin(),
                                              revenueByFlight.end());

    // Sort by revenue in descending order (STL algorithm: sort)
    sort(revenueList.begin(), revenueList.end(),
        [](const pair<string, double>& a, const pair<string, double>& b) {
            return a.second > b.second;
        });

    cout << left << setw(15) << "Flight" << "Revenue" << endl;
    cout << string(30, '-') << endl;

    int count = 0;
    for (const auto& entry : revenueList) {
        if (count >= 5) break;
        cout << left << setw(15) << entry.first
             << "$" << fixed << setprecision(2) << entry.second << endl;
        count++;
    }

    if (revenueList.empty()) {
        cout << "No revenue data available." << endl;
    }
}

// ===================== FILE PERSISTENCE =====================

// Save all data to file
void Airline::saveToFile(const string& filename) const {
    ofstream file(filename);
    if (!file.is_open()) {
        cout << "Error: Cannot open file for saving!" << endl;
        return;
    }

    // Save airline name and next ticket ID
    file << airlineName << endl;
    file << nextTicketID << endl;

    // Save flights
    file << flights.size() << endl;
    for (const auto& f : flights) {
        file << f->toFileString() << endl;
    }

    // Save passengers
    file << passengers.size() << endl;
    for (const auto& p : passengers) {
        file << p->getPassengerType() << "," << p->toFileString() << endl;
    }

    // Save tickets
    file << tickets.size() << endl;
    for (const auto& t : tickets) {
        file << t.toFileString() << endl;
    }

    // Save revenue map
    file << revenueByFlight.size() << endl;
    for (const auto& entry : revenueByFlight) {
        file << entry.first << "," << entry.second << endl;
    }

    file.close();
    cout << "Data saved successfully to " << filename << endl;
}

// Load all data from file
void Airline::loadFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "No saved data found. Starting fresh." << endl;
        return;
    }

    string line;

    try {

    // Read airline name
    getline(file, airlineName);

    // Read next ticket ID
    getline(file, line);
    nextTicketID = stoi(line);

    // Clear existing data
    flights.clear();
    passengers.clear();
    tickets.clear();
    revenueByFlight.clear();

    // Read flights
    getline(file, line);
    int flightCount = stoi(line);
    for (int i = 0; i < flightCount; i++) {
        getline(file, line);
        stringstream ss(line);
        string type, fNum, orig, dest, date, time;
        int total, available;

        getline(ss, type, ',');
        getline(ss, fNum, ',');
        getline(ss, orig, ',');
        getline(ss, dest, ',');
        getline(ss, date, ',');
        getline(ss, time, ',');

        string totalStr, availStr;
        getline(ss, totalStr, ',');
        getline(ss, availStr, ',');
        total = stoi(totalStr);
        available = stoi(availStr);

        if (type == "Domestic") {
            string region;
            getline(ss, region); // last field, no comma delimiter
            flights.push_back(make_shared<DomesticFlight>(
                fNum, orig, dest, date, time, total, available, region));
        } else if (type == "International") {
            string visaStr, country;
            getline(ss, visaStr, ',');
            getline(ss, country); // last field, no comma delimiter
            bool visa = (visaStr == "1");
            flights.push_back(make_shared<InternationalFlight>(
                fNum, orig, dest, date, time, total, available, visa, country));
        } else if (type == "Charter") {
            string holder, priceStr;
            getline(ss, holder, ',');
            getline(ss, priceStr); // last field, no comma delimiter
            double price = stod(priceStr);
            flights.push_back(make_shared<CharterFlight>(
                fNum, orig, dest, date, time, total, available, holder, price));
        }
    }

    // Read passengers
    getline(file, line);
    int passengerCount = stoi(line);
    for (int i = 0; i < passengerCount; i++) {
        getline(file, line);
        stringstream ss(line);
        string type, id, name, phone, email, pointsStr;

        getline(ss, type, ',');
        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, phone, ',');
        getline(ss, email, ',');
        getline(ss, pointsStr); // last field, no comma delimiter

        shared_ptr<Passenger> p;
        if (type == "Economy") {
            p = make_shared<EconomyPassenger>(id, name, phone, email);
        } else if (type == "Business") {
            p = make_shared<BusinessPassenger>(id, name, phone, email);
        } else if (type == "FirstClass") {
            p = make_shared<FirstClassPassenger>(id, name, phone, email);
        }

        if (p) {
            p->addLoyaltyPoints(stoi(pointsStr));
            passengers.push_back(p);
        }
    }

    // Read tickets
    getline(file, line);
    int ticketCount = stoi(line);
    for (int i = 0; i < ticketCount; i++) {
        getline(file, line);
        stringstream ss(line);
        string tID, pID, fNum, seat, fareStr, status, bDate;

        getline(ss, tID, ',');
        getline(ss, pID, ',');
        getline(ss, fNum, ',');
        getline(ss, seat, ',');
        getline(ss, fareStr, ',');
        getline(ss, status, ',');
        getline(ss, bDate); // last field, no comma delimiter

        double fare = stod(fareStr);
        tickets.push_back(Ticket(tID, pID, fNum, seat, fare, status, bDate));
    }

    // Read revenue map
    getline(file, line);
    int revenueCount = stoi(line);
    for (int i = 0; i < revenueCount; i++) {
        getline(file, line);
        stringstream ss(line);
        string fNum, revStr;
        getline(ss, fNum, ',');
        getline(ss, revStr); // last field, no comma delimiter
        revenueByFlight[fNum] = stod(revStr);
    }

    } catch (const exception& e) {
        cout << "Warning: Error reading save file (' " << e.what() << "'). Some data may not be loaded." << endl;
    }

    file.close();
    cout << "Data loaded successfully from " << filename << endl;
}

// Getters
const vector<shared_ptr<Flight>>& Airline::getFlights() const { return flights; }
const vector<shared_ptr<Passenger>>& Airline::getPassengers() const { return passengers; }
const vector<Ticket>& Airline::getTickets() const { return tickets; }
