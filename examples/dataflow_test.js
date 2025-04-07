// Sample JavaScript code to demonstrate dataflow analysis optimizations

// Example function with constant propagation opportunities
function calculateTotal(price, quantity) {
    const tax = 0.07;
    const discount = 0.1;
    
    // Constants that can be propagated
    const taxRate = tax;
    const discountRate = discount;
    
    // Common subexpression: price * quantity appears twice
    const subtotal = price * quantity;
    const taxAmount = subtotal * taxRate;
    const discountAmount = subtotal * discountRate;
    
    // More common subexpressions
    const priceWithTax = subtotal + taxAmount;
    const total = priceWithTax - discountAmount;
    
    // Dead code: this value is never used
    const deadVariable = price * 2;
    
    return total;
}

// Example function with conditional constant propagation opportunities
function processOrder(itemCount) {
    // This can be constant-folded
    const processingFee = 5 + 2.5;
    
    let shipping = 0;
    // Branch with constant condition that can be eliminated
    if (true) {
        shipping = 4.99;
    } else {
        shipping = 9.99; // Dead code
    }
    
    // Variable that gets assigned multiple times
    let discount = 0;
    if (itemCount > 5) {
        discount = itemCount * 0.1;
    } else {
        discount = itemCount * 0.05;
    }
    
    const basePrice = itemCount * 10;
    
    // Common subexpression
    const total = basePrice + shipping + processingFee - discount;
    const displayTotal = basePrice + shipping + processingFee - discount;
    
    return displayTotal;
}