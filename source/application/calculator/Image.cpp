#include <QColor>
#include <QString>
#include <QImage>

#include "Image.h"

#include <iostream>


Image::Image(QString fileName)
    : QImage(), countries()
{
    load(fileName);
    addCountries();

}
QString Image::country(int x, int y)
{
    QRgb color = pixel(x, y);
    int gray = qGray(color);

    QString country = (QString)countries.at(gray);
    return country;
}
void Image::addCountries()
{
    countries << "Afghanistan";
    countries << "Albania";
    countries << "Algeria";
    countries << "Andorra";
    countries << "EMPTY";
    countries << "Angola";
    countries << "EMPTY";
    countries << "Antarctica";
    countries << "Argentina";
    countries << "EMPTY";
    countries << "Armenia";
    countries << "EMPTY";
    countries << "Australia";
    countries << "Austria";
    countries << "Azerbaijan";
    countries << "EMPTY";
    countries << "Bahrain";
    countries << "Bangladesh";
    countries << "Belarus";
    countries << "EMPTY";
    countries << "Belgium";
    countries << "EMPTY";
    countries << "EMPTY";
    countries << "Belize";
    countries << "Benin";
    countries << "EMPTY";
    countries << "Bhutan";
    countries << "Bolivia";
    countries << "EMPTY";
    countries << "Bosnia and Herzegovina";
    countries << "Botswana";
    countries << "EMPTY";
    countries << "EMPTY";
    countries << "Brazil";
    countries << "EMPTY";
    countries << "Bulgaria";
    countries << "Burkina";
    countries << "Burma";
    countries << "EMPTY";
    countries << "Burundi";
    countries << "EMPTY";
    countries << "CAR";
    countries << "Cambodia";
    countries << "Cabinda";
    countries << "Cameroon";
    countries << "EMPTY";
    countries << "Canada";
    countries << "Chad";
    countries << "EMPTY";
    countries << "EMPTY";
    countries << "Chile";
    countries << "China";
    countries << "Colombia";
    countries << "EMPTY";
    countries << "Congo";
    countries << "EMPTY";
    countries << "Costa Rica";
    countries << "Cote d'Ivoire";
    countries << "Croatia";
    countries << "EMPTY";
    countries << "Cuba";
    countries << "Cyprus";
    countries << "EMPTY";
    countries << "Czech Republic";
    countries << "Denmark";
    countries << "Djibouti";
    countries << "EMPTY";
    countries << "Dominican Republic";
    countries << "Ecuador";
    countries << "EMPTY";
    countries << "Egypt";
    countries << "El Salvador";
    countries << "EMPTY";
    countries << "Equatorial Guinea";
    countries << "Eritrea";
    countries << "EMPTY";
    countries << "Ethiopia";
    countries << "Estonia";
    countries << "Falkland Islands";
    countries << "EMPTY";
    countries << "Fiji";
    countries << "Finland";
    countries << "EMPTY";
    countries << "France";
    countries << "French Guiana";
    countries << "EMPTY";
    countries << "EMPTY";
    countries << "Gabon";
    countries << "EMPTY";
    countries << "Georgia";
    countries << "Germany";
    countries << "Ghana";
    countries << "EMPTY";
    countries << "EMPTY";
    countries << "Gibraltar";
    countries << "Greece";
    countries << "Greenland";
    countries << "EMPTY";
    countries << "Guatemala";
    countries << "Guinea";
    countries << "Guinea-Bissau";
    countries << "EMPTY";
    countries << "Guyana";
    countries << "EMPTY";
    countries << "Haiti";
    countries << "Honduras";
    countries << "Hungary";
    countries << "EMPTY";
    countries << "Iceland";
    countries << "India";
    countries << "EMPTY";
    countries << "Indonesia";
    countries << "Iran";
    countries << "EMPTY";
    countries << "Iraq";
    countries << "Ireland";
    countries << "Israel";
    countries << "Italy";
    countries << "EMPTY";
    countries << "Jamaica";
    countries << "EMPTY";
    countries << "Japan";
    countries << "Jordan";
    countries << "EMPTY";
    countries << "Kazakhstan";
    countries << "Kenya";
    countries << "EMPTY";
    countries << "Kuwait";
    countries << "Kyrgyzstan";
    countries << "EMPTY";
    countries << "Laos";
    countries << "Latvia";
    countries << "EMPTY";
    countries << "Lebanon";
    countries << "Liberia";
    countries << "EMPTY";
    countries << "Libya";
    countries << "EMPTY";
    countries << "Liechtenstein";
    countries << "Lithuania";
    countries << "Luxembourg";
    countries << "Macedonia";
    countries << "EMPTY";
    countries << "Madagascar";
    countries << "Malawi";
    countries << "EMPTY";
    countries << "EMPTY";
    countries << "Malaysia";
    countries << "Mali";
    countries << "Mauritania";
    countries << "Mexico";
    countries << "Moldova";
    countries << "EMPTY";
    countries << "Monaco";
    countries << "EMPTY";
    countries << "Mongolia";
    countries << "Morocco";
    countries << "Mozambique";
    countries << "EMPTY";
    countries << "Namibia";
    countries << "Nepal";
    countries << "Netherlands";
    countries << "EMPTY";
    countries << "EMPTY";
    countries << "New Zealand";
    countries << "Nicaragua";
    countries << "Niger";
    countries << "EMPTY";
    countries << "Nigeria";
    countries << "EMPTY";
    countries << "North Korea";
    countries << "Norway";
    countries << "Oman";
    countries << "EMPTY";
    countries << "Pakistan";
    countries << "Panama";
    countries << "Papua New Guinea";
    countries << "EMPTY";
    countries << "Paraguay";
    countries << "Peru";
    countries << "EMPTY";
    countries << "Philippines";
    countries << "Poland";
    countries << "EMPTY";
    countries << "Portugal";
    countries << "Puerto Rico";
    countries << "EMPTY";
    countries << "Qatar";
    countries << "Romania";
    countries << "EMPTY";
    countries << "EMPTY";
    countries << "Russia";
    countries << "Rwanda";
    countries << "Saudi Arabia";
    countries << "San Marino";
    countries << "Senegal";
    countries << "EMPTY";
    countries << "Serbia and Montenegro";
    countries << "Sierra Leone";
    countries << "EMPTY";
    countries << "EMPTY";
    countries << "Singapore";
    countries << "Slovakia";
    countries << "Slovenia";
    countries << "Somalia";
    countries << "South Africa";
    countries << "EMPTY";
    countries << "EMPTY";
    countries << "Lesotho";
    countries << "South Korea";
    countries << "Sri Lanka";
    countries << "Spain";
    countries << "Sudan";
    countries << "EMPTY";
    countries << "Suriname";
    countries << "EMPTY";
    countries << "Swaziland";
    countries << "EMPTY";
    countries << "Sweden";
    countries << "Switzerland";
    countries << "Syria";
    countries << "EMPTY";
    countries << "Taiwan";
    countries << "EMPTY";
    countries << "Tajikistan";
    countries << "Tanzania";
    countries << "Thailand";
    countries << "EMPTY";
    countries << "Togo";
    countries << "Tunisia";
    countries << "Turkmenistan";
    countries << "Turkey";
    countries << "UAE";
    countries << "EMPTY";
    countries << "EMPTY";
    countries << "United Kingdom";
    countries << "USA";
    countries << "EMPTY";
    countries << "Uganda";
    countries << "Ukraine";
    countries << "EMPTY";
    countries << "Uruguay";
    countries << "Uzbekistan";
    countries << "EMPTY";
    countries << "EMPTY";
    countries << "Venezuela";
    countries << "Vietnam";
    countries << "Western Sahara";
    countries << "Yemen";
    countries << "EMPTY";
    countries << "Zaire";
    countries << "EMPTY";
    countries << "Zambia";
    countries << "Zimbabwe";
    countries << "EMPTY";
    countries << "EMPTY";
}
